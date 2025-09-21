#include "parse.h"

#include <stdio.h> /* TODO debug only */
#include <string.h>

#define CEL_EVAL1(x) x
#define CEL_EVAL2(x) CEL_EVAL1(CEL_EVAL1(x))
#define CEL_EVAL4(x) CEL_EVAL2(CEL_EVAL2(x))
#define CEL_EVAL8(x) CEL_EVAL4(CEL_EVAL4(x))
#define CEL_EVAL(x) CEL_EVAL8(CEL_EVAL8(x))

#define CEL_PARENS ()

#define parse_require(block, ...) PARSE_REQUIRE(false, block, __VA_ARGS__)
#define PARSE_REQUIRE(fail, block, ...)                                        \
 {                                                                             \
  bool retval = true;                                                          \
  retval = CEL_EVAL(parse_seq(block, __VA_ARGS__));                            \
  if (!retval) {                                                               \
   return fail;                                                                \
  }                                                                            \
 }

#define parse_try(block, ...) PARSE_TRY(true, block, __VA_ARGS__)
#define PARSE_TRY(success, block, ...)                                         \
 {                                                                             \
  const cel_char *mark = span->end;                                            \
  bool retval = true;                                                          \
  retval = CEL_EVAL(parse_seq(block, __VA_ARGS__));                            \
  if (retval) {                                                                \
   return success;                                                             \
  }                                                                            \
  span->end = mark;                                                            \
 }

#define parse_try_call(call)                                                   \
 {                                                                             \
  const cel_char *mark = span->end;                                            \
  int retval = call;                                                           \
  if (retval) {                                                                \
   return retval;                                                              \
  }                                                                            \
  span->end = mark;                                                            \
 }

#define parse_seq(block, ...)                                                  \
 block __VA_OPT__(; CEL_EVAL(PARSE_SEQ_BEGIN(__VA_ARGS__)))
#define PARSE_SEQ_BEGIN(...)                                                   \
 do {                                                                          \
  PARSE_SEQ(__VA_ARGS__);                                                      \
 } while (0)
#define PARSE_SEQ(block, ...)                                                  \
 if (!retval) {                                                                \
  break;                                                                       \
 }                                                                             \
 retval = block __VA_OPT__(; PARSE_SEQ_AGAIN CEL_PARENS(__VA_ARGS__))
#define PARSE_SEQ_AGAIN() PARSE_SEQ

#define parse_or(block, ...)                                                   \
 retval;                                                                       \
 do {                                                                          \
  const cel_char *mark = span->end;                                            \
  PARSE_OR(block, __VA_ARGS__);                                                \
 } while (0)
#define PARSE_OR(block, ...)                                                   \
 retval = block;                                                               \
 if (retval) {                                                                 \
  break;                                                                       \
 }                                                                             \
 span->end = mark __VA_OPT__(; PARSE_OR_AGAIN CEL_PARENS(__VA_ARGS__))
#define PARSE_OR_AGAIN() PARSE_OR

#define parse_not(block)                                                       \
 retval;                                                                       \
 {                                                                             \
  const cel_char *mark = span->end;                                            \
  retval = block;                                                              \
  span->end = mark;                                                            \
  retval = !retval;                                                            \
 }

#define parse_repeat(block, lo, hi)                                            \
 retval;                                                                       \
 do {                                                                          \
  const cel_char *mark = span->end;                                            \
  for (size_t i = 0; i < lo; i++) {                                            \
   if (!retval) {                                                              \
    break;                                                                     \
   }                                                                           \
   retval = block;                                                             \
   if (span->end == mark) {                                                    \
    retval = false;                                                            \
   }                                                                           \
   mark = span->end;                                                           \
  }                                                                            \
  if (!retval) {                                                               \
   break;                                                                      \
  }                                                                            \
  for (size_t i = lo; i < hi; i++) {                                           \
   mark = span->end;                                                           \
   retval = block;                                                             \
   if (!retval || span->end == mark) {                                         \
    span->end = mark;                                                          \
    break;                                                                     \
   }                                                                           \
  }                                                                            \
  retval = true;                                                               \
 } while (0)

#define parse_count(block, count) parse_repeat(block, count, count)
#define parse_opt(block) parse_repeat(block, 0, 1)
#define parse_star(block) parse_repeat(block, 0, SIZE_MAX)
#define parse_plus(block) parse_repeat(block, 1, SIZE_MAX)

#define parse_encap(open, enumerator, mid, close)                              \
 open, enumerator(parse_seq(parse_not(close), mid)), close
#define parse_encap_star(open, mid, close)                                     \
 parse_encap(open, parse_star, mid, close)
#define parse_encap_plus(open, mid, close)                                     \
 parse_encap(open, parse_plus, mid, close)

#define parse_capture(capture_span, ...)                                       \
 retval;                                                                       \
 {                                                                             \
  capture_span->start = span->end;                                             \
  capture_span->end = span->end;                                               \
  cel_pp_parse_span *hold = span;                                              \
  span = capture_span;                                                         \
  retval = CEL_EVAL(parse_seq(__VA_ARGS__));                                   \
  span = hold;                                                                 \
  span->end = capture_span->end;                                               \
 }

static bool parse_dot(cel_pp_parse_span *span)
{
 if (*span->end) {
  span->end++;
  return true;
 }
 return false;
}

static bool parse_char(cel_pp_parse_span *span, char c)
{
 if (*span->end == (cel_char)c) {
  span->end++;
  return true;
 }
 return false;
}

static bool parse_urange(cel_pp_parse_span *span, cel_char lo, cel_char hi)
{
 if (*span->end >= lo && *span->end <= hi) {
  span->end++;
  return true;
 }
 return false;
}

static bool parse_range(cel_pp_parse_span *span, char lo, char hi)
{
 return parse_urange(span, lo, hi);
}

static bool parse_set(cel_pp_parse_span *span, const char *set)
{
 for (size_t i = 0; set[i]; i++) {
  if (parse_char(span, set[i])) {
   return true;
  }
 }
 return false;
}

static bool parse_cstr(cel_pp_parse_span *span, const char *cstr)
{
 for (size_t i = 0; cstr[i]; i++) {
  if (!parse_char(span, cstr[i])) {
   return false;
  }
 }
 return true;
}

static bool parse_cstr_set(cel_pp_parse_span *span, const char *const *set)
{
 const cel_char *mark = span->end;
 for (size_t i = 0; set[i]; i++) {
  if (parse_cstr(span, set[i])) {
   return true;
  }
  span->end = mark;
 }
 return false;
}

static bool parse_newline(cel_pp_parse_span *span)
{
 static const char *newline[] = {"\r\n", "\r", "\n", NULL};
 return parse_cstr_set(span, newline);
}

static bool parse_end_of_line(cel_pp_parse_span *span)
{
 parse_try(parse_newline(span));
 parse_try(parse_char(span, 0));
 return false;
}

static bool parse_to_end_of_line(cel_pp_parse_span *span)
{
 parse_try(parse_star(
     parse_seq(parse_not(parse_end_of_line(span)), parse_dot(span))));
 return true;
}

static bool parse_esc_newline(cel_pp_parse_span *span)
{
 return parse_char(span, '\\') && parse_newline(span);
}

static bool parse_multiline_comment(cel_pp_parse_span *span)
{
 parse_try(parse_encap_star(parse_cstr(span, "/*"), parse_dot(span),
			    parse_cstr(span, "*/")));
 return false;
}

static bool parse_singleline_comment(cel_pp_parse_span *span)
{
 parse_try(parse_encap_star(parse_cstr(span, "//"),
			    parse_or(parse_esc_newline(span), parse_dot(span)),
			    parse_end_of_line(span)));
 return false;
}

static bool parse_whitespace(cel_pp_parse_span *span)
{
 parse_try(parse_plus(
     parse_or(parse_esc_newline(span), parse_multiline_comment(span),
	      parse_singleline_comment(span), parse_set(span, " \f\t\v"))));
 return false;
}

static bool parse_whitespace_opt(cel_pp_parse_span *span)
{
 parse_try(parse_whitespace(span));
 return true;
}

static bool parse_empty_line(cel_pp_parse_span *span)
{
 parse_try(parse_whitespace_opt(span), parse_end_of_line(span));
 return false;
}

static bool parse_header_name(cel_pp_parse_span *span)
{
 parse_whitespace_opt(span);
 parse_try(parse_encap_plus(
     parse_char(span, '<'),
     parse_seq(parse_not(parse_newline(span)), parse_dot(span)),
     parse_char(span, '>')));
 parse_try(parse_encap_plus(
     parse_char(span, '"'),
     parse_seq(parse_not(parse_newline(span)), parse_dot(span)),
     parse_char(span, '"')));
 return false;
}

static bool parse_digit(cel_pp_parse_span *span)
{
 return parse_range(span, '0', '9');
}

static bool parse_hex(cel_pp_parse_span *span)
{
 parse_try(parse_digit(span));
 parse_try(parse_range(span, 'a', 'f'));
 parse_try(parse_range(span, 'A', 'F'));
 return false;
}

static bool parse_universal_character_name(cel_pp_parse_span *span)
{
 parse_try(parse_cstr(span, "\\u"), parse_count(parse_hex(span), 4));
 parse_try(parse_cstr(span, "\\U"), parse_count(parse_hex(span), 8));
 return false;
}

static bool parse_other_identifier_char(cel_pp_parse_span *span)
{
 /* TODO stub */
 return false;
}

static bool parse_identifier_nondigit(cel_pp_parse_span *span)
{
 parse_try(parse_char(span, '_'));
 parse_try(parse_range(span, 'a', 'z'));
 parse_try(parse_range(span, 'A', 'Z'));
 parse_try(parse_universal_character_name(span));
 parse_try(parse_other_identifier_char(span));
 return false;
}

static bool parse_identifier(cel_pp_parse_span *span)
{
 parse_try(
     parse_identifier_nondigit(span),
     parse_star(parse_or(parse_identifier_nondigit(span), parse_digit(span))));
 return false;
}

static bool parse_pp_number(cel_pp_parse_span *span)
{
 parse_try(parse_or(parse_digit(span),
		    parse_seq(parse_char(span, '.'), parse_digit(span))),
	   parse_star(parse_or(
	       parse_digit(span), parse_identifier_nondigit(span),
	       parse_seq(parse_set(span, "eEpP"), parse_set(span, "+-")),
	       parse_char(span, '.'))));
 return false;
}

static bool parse_escape_sequence(cel_pp_parse_span *span)
{
 parse_try(parse_universal_character_name(span));
 parse_require(parse_char(span, '\\'));
 parse_try(parse_set(span, "'\"?\\abfnrtv"));
 parse_try(parse_repeat(parse_range(span, '0', '7'), 1, 3));
 parse_try(parse_char(span, 'x'), parse_plus(parse_hex(span)));
 return false;
}

static bool parse_character_constant(cel_pp_parse_span *span)
{
 parse_try(
     parse_opt(parse_set(span, "LuU")),
     parse_encap_plus(
	 parse_char(span, '\''),
	 parse_or(parse_escape_sequence(span),
		  parse_seq(parse_not(parse_char(span, '\\')),
			    parse_not(parse_newline(span)), parse_dot(span))),
	 parse_char(span, '\'')));
 return false;
}

static bool parse_string_literal(cel_pp_parse_span *span)
{
 static const char *encoding_prefix[] = {"u8", "u", "U", "L", NULL};
 parse_try(
     parse_opt(parse_cstr_set(span, encoding_prefix)),
     parse_encap_star(
	 parse_char(span, '"'),
	 parse_or(parse_escape_sequence(span),
		  parse_seq(parse_not(parse_char(span, '\\')),
			    parse_not(parse_newline(span)), parse_dot(span))),
	 parse_char(span, '"')));
 return false;
}

static bool parse_punctuator(cel_pp_parse_span *span)
{
 static const char *punctuator[] = {
     "%:%:", "...", "<<=", ">>=", "->", "++", "--", ">>", "<<", "<=", ">=",
     "==",   "!=",  "&&",  "||",  "*=", "/=", "%=", "+=", "-=", "&=", "^=",
     "|=",   "##",  "<:",  ":>",  "<%", "%>", "%:", "[",  "]",	"(",  ")",
     "{",    "}",   ".",   "&",	  "*",	"+",  "-",  "~",  "!",	"/",  "%",
     "<",    ">",   "^",   "|",	  "?",	":",  ";",  "=",  ",",	"#",  NULL};
 return parse_cstr_set(span, punctuator);
}

#define parse_try_type(type, block, ...) PARSE_TRY(type, block, __VA_ARGS__)

static cel_pp_parse_type parse_pp_token(cel_pp_parse_span *span)
{
 parse_whitespace_opt(span);
 parse_try_type(CEL_PP_PARSE_END_OF_LINE, parse_end_of_line(span));
 parse_try_type(CEL_PP_PARSE_IDENTIFIER, parse_identifier(span));
 parse_try_type(CEL_PP_PARSE_PP_NUMBER, parse_pp_number(span));
 parse_try_type(CEL_PP_PARSE_CHARACTER_CONSTANT,
		parse_character_constant(span));
 parse_try_type(CEL_PP_PARSE_STRING_LITERAL, parse_string_literal(span));
 parse_try_type(CEL_PP_PARSE_PUNCTUATOR, parse_punctuator(span));
 parse_try_type(CEL_PP_PARSE_OTHER_CHAR, parse_dot(span));
 return CEL_PP_PARSE_END_OF_LINE;
}

static bool parse_token_is_type(cel_pp_parse_span *span, cel_pp_parse_type type)
{
 return parse_pp_token(span) == type;
}

static bool parse_token_is_char(cel_pp_parse_span *span, char c)
{
 parse_whitespace_opt(span);
 const cel_char *mark = span->end;
 parse_pp_token(span);
 return ((cel_uptr)span->end - (cel_uptr)mark) == 1 && *mark == (cel_char)c;
}

static bool parse_token_is_cstr(cel_pp_parse_span *span, const char *cstr)
{
 parse_whitespace_opt(span);

 const cel_char *mark = span->end;
 size_t len = strlen(cstr);
 parse_pp_token(span);

 if ((cel_uptr)span->end - (cel_uptr)mark != len) {
  return false;
 }
 return memcmp(mark, cstr, len) == 0;
}

#define parse_require_type(...) PARSE_REQUIRE(CEL_PP_PARSE_FAIL, __VA_ARGS__)

static cel_pp_parse_type parse_include_line(cel_pp_parse_span *span,
					    cel_pp_parse_span *header)
{
 parse_require_type(parse_token_is_char(span, '#'),
		    parse_token_is_cstr(span, "include"));
 parse_try_type(CEL_PP_PARSE_INCLUDE_HEADER_NAME,
		parse_capture(header, parse_header_name(span)));
 parse_try_type(CEL_PP_PARSE_INCLUDE_HEADER_EXPAND,
		parse_capture(header, parse_to_end_of_line(span)));
 return CEL_PP_PARSE_FAIL;
}

cel_pp_parse_type parse_define_line(cel_pp_parse_span *span,
				    cel_pp_parse_span *identifier,
				    cel_pp_parse_span *identifier_list,
				    cel_pp_parse_span *replacement_list)
{
 parse_require_type(
     parse_token_is_cstr(span, "define"),
     parse_capture(identifier,
		   parse_token_is_type(span, CEL_PP_PARSE_IDENTIFIER)));
 parse_try_type(
     CEL_PP_PARSE_DEFINE_MACRO_FN, parse_char(span, '('),
     parse_capture(
	 identifier_list,
	 parse_opt(parse_seq(
	     parse_star(
		 parse_seq(parse_token_is_type(span, CEL_PP_PARSE_IDENTIFIER),
			   parse_token_is_char(span, ','))),
	     parse_or(parse_token_is_cstr(span, "..."),
		      parse_token_is_type(span, CEL_PP_PARSE_IDENTIFIER))))),
     parse_token_is_char(span, ')'),
     parse_capture(replacement_list, parse_to_end_of_line(span)));
 parse_try_type(CEL_PP_PARSE_DEFINE_MACRO_OBJ,
		parse_capture(replacement_list, parse_to_end_of_line(span)));
 return CEL_PP_PARSE_FAIL;
}

bool parse_undef_line(cel_pp_parse_span *span, cel_pp_parse_span *identifier)
{
 parse_require(parse_token_is_cstr(span, "undef"));
 parse_try(parse_capture(identifier, parse_identifier(span)),
	   parse_empty_line(span));
 return false;
}

bool parse_ifdef_line(cel_pp_parse_span *span, cel_pp_parse_span *identifier)
{
 parse_require(parse_token_is_cstr(span, "ifdef"));
 parse_try(parse_capture(identifier, parse_identifier(span)),
	   parse_empty_line(span));
 return false;
}

bool parse_ifndef_line(cel_pp_parse_span *span, cel_pp_parse_span *identifier)
{
 parse_require(parse_token_is_cstr(span, "ifndef"));
 parse_try(parse_capture(identifier, parse_identifier(span)),
	   parse_empty_line(span));
 return false;
}

"%:%:", "...", "<<=", ">>=", "->", "++", "--", ">>", "<<",
    "<=", ">=", "==", "!=", "&&", "||",
    "*=", "/=", "%=", "+=", "-=", "&=", "^=", "|=", "##", "<:", ":>", "<%",
    "%>", "%:", "[", "]", "(", ")", "{", "}", ".", "&", "*", "+", "-", "~", "!",
    "/", "%", "<", ">", "^", "|", "?", ":", ";", "=", ",", "#", NULL
}
;
bool parse_conditional_expression(cel_pp_parse_span *span)
{
 static const char *infix_ops[] = {"<<", ">>", "<=", ">=", "==", "!=", "&&",
				   "||", "*",  "/",  "%",  "+",	 "-",  "<",
				   ">",	 "&",  "^",  "|",  NULL};
 static const char *prefix_ops[] = {"+", "-", "~", "!", NULL};
}

cel_pp_parse_type parse_pp_directive(cel_pp_parse_span *span,
				     cel_pp_parse_span **capture)
{
 parse_require(parse_token_is_char(span, '#'));
 parse_try_call(parse_include_line(span, capture[0]));
 parse_try_call(parse_define_line(span, capture[0], capture[1], capture[2]));
 parse_try_type(CEL_PP_PARSE_UNDEF, parse_undef_line(span, capture[0]));
 parse_try_type(CEL_PP_PARSE_IFDEF, parse_ifdef_line(span, capture[0]));
 parse_try_type(CEL_PP_PARSE_IFNDEF, parse_ifndef_line(span, capture[0]));
}
