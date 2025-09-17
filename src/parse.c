#include "parse.h"

#include <stdarg.h>

typedef bool (*cel_pp_parse_fn)(const cel_char **, const void *);

static bool parse_dot(const cel_char **src_p)
{
	const cel_char *src = *src_p;
	if (*src) {
		*src_p = src + 1;
		return true;
	}
	return false;
}

static bool parse_char(const cel_char **src_p, char c)
{
	const cel_char *src = *src_p;
	if (*src == (cel_char)c) {
		*src_p = src + 1;
		return true;
	}
	return false;
}

static bool parse_not_char(const cel_char **src_p, char c)
{
	const cel_char *src = *src_p;
	return !parse_char(&src, c);
}

static bool
parse_unsigned_range(const cel_char **src_p, cel_char lo, cel_char hi)
{
	const cel_char *src = *src_p;
	if (*src >= lo && *src <= hi) {
		*src_p = src + 1;
		return true;
	}
	return false;
}

static bool parse_range(const cel_char **src_p, char lo, char hi)
{
	return parse_unsigned_range(src_p, (cel_char)lo, (cel_char)hi);
}

static bool parse_not_range(const cel_char **src_p, char lo, char hi)
{
	const cel_char *src = *src_p;
	return !parse_range(&src, lo, hi);
}

static bool parse_set(const cel_char **src_p, const char *set)
{
	for (size_t i = 0; set[i]; i++) {
		const cel_char *src = *src_p;
		if (parse_char(&src, set[i])) {
			*src_p = src;
			return true;
		}
	}
	return false;
}

static bool parse_not_set(const cel_char **src_p, const char *set)
{
	const cel_char *src = *src_p;
	return !parse_set(&src, set);
}

static bool parse_cstr(const cel_char **src_p, const char *cstr)
{
	const cel_char *src = *src_p;
	for (size_t i = 0; cstr[i]; i++) {
		if (!parse_char(&src, cstr[i])) {
			return false;
		}
	}

	*src_p = src;
	return true;
}

static bool parse_not_cstr(const cel_char **src_p, const char *cstr)
{
	const cel_char *src = *src_p;
	return !parse_cstr(&src, cstr);
}

static bool parse_cstr_set(const cel_char **src_p, const char *const *cstr_set)
{
	for (size_t i = 0; cstr_set[i]; i++) {
		const cel_char *src = *src_p;
		if (parse_cstr(&src, cstr_set[i])) {
			*src_p = src;
			return true;
		}
	}
	return false;
}

static bool parse_not(const cel_char **src_p,
                      const bool (*fn)(const cel_char **))
{
	const cel_char *src = *src_p;
	if (fn(&src)) {
		return false;
	}
	return true;
}

#define CEL_PP_PARSE_INF SIZE_MAX

static bool parse_repeat(const cel_char **src_p,
                         const bool (*fn)(const cel_char **),
                         size_t lo,
                         size_t hi)
{
	const cel_char *src = *src_p;
	for (size_t i = 0; i < lo; i++) {
		if (!fn(&src)) {
			return false;
		}
	}
	*src_p = src;

	for (size_t i = lo; i < hi; i++) {
		if (!fn(src_p)) {
			return true;
		}
	}
	return true;
}

static bool parse_repeat_not(const cel_char **src_p,
                             const bool (*fn)(const cel_char **),
                             size_t lo,
                             size_t hi)
{
	const cel_char *src = *src_p;
	for (size_t i = 0; i < lo; i++) {
		if (!parse_not(&src, fn)) {
			return false;
		}
	}
	*src_p = src;

	for (size_t i = lo; i < hi; i++) {
		if (!parse_not(&src, fn)) {
			return true;
		}
	}
	return true;
}

static bool parse_count(const cel_char **src_p,
                        const bool (*fn)(const cel_char **),
                        size_t count)
{
	return parse_repeat(src_p,
	                    fn,
	                    count,
	                    count < CEL_PP_PARSE_INF ? count + 1
	                                             : CEL_PP_PARSE_INF);
}

static bool parse_count_not(const cel_char **src_p,
                            const bool (*fn)(const cel_char **),
                            size_t count)
{
	return parse_repeat_not(src_p,
	                        fn,
	                        count,
	                        count < CEL_PP_PARSE_INF ? count + 1
	                                                 : CEL_PP_PARSE_INF);
}

static bool parse_optional(const cel_char **src_p,
                           const bool (*fn)(const cel_char **))
{
	return parse_repeat(src_p, fn, 0, 1);
}

static bool parse_star(const cel_char **src_p,
                       const bool (*fn)(const cel_char **))
{
	return parse_repeat(src_p, fn, 0, CEL_PP_PARSE_INF);
}

static bool parse_star_not(const cel_char **src_p,
                           const bool (*fn)(const cel_char **))
{
	return parse_repeat_not(src_p, fn, 0, CEL_PP_PARSE_INF);
}

static bool parse_plus(const cel_char **src_p,
                       const bool (*fn)(const cel_char **))
{
	return parse_repeat(src_p, fn, 1, CEL_PP_PARSE_INF);
}

static bool parse_plus_not(const cel_char **src_p,
                           const bool (*fn)(const cel_char **))
{
	return parse_repeat_not(src_p, fn, 1, CEL_PP_PARSE_INF);
}

static bool parse_newline(const cel_char **src_p)
{
	static const char *newline_set[] = {"\r\n", "\r", "\n", NULL};
	return parse_cstr_set(src_p, newline_set);
}

static bool parse_escaped_newline(const cel_char **src_p)
{
	const cel_char *src = *src_p;
	if (parse_char(&src, '\\') && parse_newline(&src)) {
		*src_p = src;
		return true;
	}
	return false;
}

#define CEL_PP_PARSE_END (cel_pp_parse_fn)0

static bool parse_multiline_comment_char(const cel_char **src_p)
{
	const cel_char *src = *src_p;
	if (parse_escaped_newline(&src)) {
		*src_p = src;
		return true;
	}
	return parse_not_cstr(src_p, "*/") && parse_dot(src_p);
}

static bool parse_multiline_comment(const cel_char **src_p)
{
	const cel_char *src = *src_p;
	if (parse_cstr(&src, "/*") && parse_star(&src, parse_multiline_comment_char)
	    && parse_cstr(&src, "*/")) {
		*src_p = src;
		return true;
	}
	return false;
}

static bool parse_singleline_comment_char(const cel_char **src_p)
{
	const cel_char *src = *src_p;
	if (parse_escaped_newline(&src)) {
		*src_p = src;
		return true;
	}
	return parse_not(src_p, parse_newline) && parse_dot(src_p);
}

static bool parse_singleline_comment(const cel_char **src_p)
{
	const cel_char *src = *src_p;
	if (parse_cstr(&src, "//")
	    && parse_star(&src, parse_singleline_comment_char)
	    && parse_newline(&src)) {
		*src_p = src;
		return true;
	}
	return false;
}

static bool parse_comment(const cel_char **src_p)
{
	return parse_multiline_comment(src_p) || parse_singleline_comment(src_p);
}

static bool parse_whitespace_item(const cel_char **src_p)
{
	return parse_comment(src_p) || parse_set(src_p, " \f\t\v");
}

static bool parse_whitespace(const cel_char **src_p)
{
	return parse_star(src_p, parse_whitespace_item);
}

static bool parse_digit(const cel_char **src_p)
{
	return parse_range(src_p, '0', '9');
}

static bool parse_hex_digit(const cel_char **src_p)
{
	return parse_digit(src_p) || parse_range(src_p, 'a', 'f')
	    || parse_range(src_p, 'A', 'F');
}

static bool parse_universal_character_name(const cel_char **src_p)
{
	const cel_char *src = *src_p;
	if (parse_cstr(&src, "\\U") && parse_count(&src, parse_hex_digit, 8)) {
		*src_p = src;
		return true;
	}

	src = *src_p;
	if (parse_cstr(&src, "\\u") && parse_count(&src, parse_hex_digit, 4)) {
		*src_p = src;
		return true;
	}
	return false;
}

static bool parse_other_identifier_char(const cel_char **src_p)
{
	/* TODO stub */
	CEL_UNUSED(src_p);
	return false;
}

static bool parse_identifier_nondigit(const cel_char **src_p)
{
	return parse_universal_character_name(src_p)
	    || parse_other_identifier_char(src_p) || parse_char(src_p, '_')
	    || parse_range(src_p, 'a', 'z') || parse_range(src_p, 'A', 'Z');
}

static bool parse_identifier_char(const cel_char **src_p)
{
	return parse_digit(src_p) || parse_identifier_nondigit(src_p);
}

static bool parse_identifier(const cel_char **src_p)
{
	const cel_char *src = *src_p;
	if (parse_identifier_nondigit(&src)
	    && parse_star(&src, parse_identifier_char)) {
		*src_p = src;
		return true;
	}
	return false;
}

static bool parse_pp_number_sign(const cel_char **src_p)
{
	const cel_char *src = *src_p;
	if (parse_set(&src, "eEpP") && parse_set(&src, "-+")) {
		*src_p = src;
		return true;
	}
	return false;
}

static bool parse_pp_number_part(const cel_char **src_p)
{
	return parse_pp_number_sign(src_p) || parse_char(src_p, '.')
	    || parse_identifier_char(src_p);
}

static bool parse_pp_number(const cel_char **src_p)
{
	const cel_char *src = *src_p;
	if (parse_digit(&src) && parse_star(&src, parse_pp_number_part)) {
		*src_p = src;
		return true;
	}

	src = *src_p;
	if (parse_char(&src, '.') && parse_digit(&src)
	    && parse_star(&src, parse_pp_number_part)) {
		*src_p = src;
		return true;
	}
	return false;
}

static bool parse_character_encoding(const cel_char **src_p)
{
	return parse_set(src_p, "LuU");
}

static bool parse_simple_escape_sequence(const cel_char **src_p)
{
	static const char *set[] = {"\\'",
	                            "\\\"",
	                            "\\?",
	                            "\\\\",
	                            "\\a",
	                            "\\b",
	                            "\\f",
	                            "\\n",
	                            "\\r",
	                            "\\t",
	                            "\\v",
	                            NULL};
	return parse_cstr_set(src_p, set);
}

static bool parse_octal_digit(const cel_char **src_p)
{
	return parse_range(src_p, '0', '7');
}

static bool parse_octal_escape_sequence(const cel_char **src_p)
{
	const cel_char *src = *src_p;
	if (parse_char(&src, '\\') && parse_repeat(&src, parse_octal_digit, 1, 3)) {
		*src_p = src;
		return true;
	}
	return false;
}

static bool parse_hexadecimal_escape_sequence(const cel_char **src_p)
{
	const cel_char *src = *src_p;
	if (parse_cstr(&src, "\\x") && parse_plus(&src, parse_hex_digit)) {
		*src_p = src;
		return true;
	}
	return false;
}

static bool parse_escape_sequence(const cel_char **src_p)
{
	return parse_simple_escape_sequence(src_p)
	    || parse_octal_escape_sequence(src_p)
	    || parse_hexadecimal_escape_sequence(src_p)
	    || parse_universal_character_name(src_p);
}

static bool parse_c_char(const cel_char **src_p)
{
	return parse_escape_sequence(src_p)
	    || (parse_not_char(src_p, '\\') && parse_not_char(src_p, '\'')
	        && parse_not(src_p, parse_newline) && parse_dot(src_p));
}

static bool parse_character_constant(const cel_char **src_p)
{
	const cel_char *src = *src_p;
	if (parse_optional(&src, parse_character_encoding) && parse_char(&src, '\'')
	    && parse_plus(&src, parse_c_char) && parse_char(&src, '\'')) {
		*src_p = src;
		return true;
	}
	return false;
}

static bool parse_encoding_prefix(const cel_char **src_p)
{
	static const char *set[] = {"u8", "u", "U", "L", NULL};
	return parse_cstr_set(src_p, set);
}

static bool parse_s_char(const cel_char **src_p)
{
	return parse_escape_sequence(src_p)
	    || (parse_not_char(src_p, '\\') && parse_not_char(src_p, '"')
	        && parse_not(src_p, parse_newline) && parse_dot(src_p));
}

static bool parse_string_literal(const cel_char **src_p)
{
	const cel_char *src = *src_p;
	if (parse_optional(&src, parse_encoding_prefix) && parse_char(&src, '"')
	    && parse_star(&src, parse_s_char) && parse_char(&src, '"')) {
		*src_p = src;
		return true;
	}
	return false;
}

static bool parse_punctuator(const cel_char **src_p)
{
	static const char *set[] = {
		"%:%:", "...", "<<=", ">>=", "++", "--", "<<", ">>", "<=", ">=", "==",
		"!=",   "&&",  "||",  "*=",  "/=", "%=", "+=", "-=", "&=", "^=", "|=",
		"##",   "<:",  ":>",  "<%",  "%>", "%:", "[",  "]",  "(",  ")",  "{",
		"}",    ".",   "&",   "*",   "+",  "-",  "~",  "!",  "/",  "%",  "<",
		">",    "^",   "|",   "?",   ":",  ";",  "=",  ",",  "#",  NULL};
	return parse_cstr_set(src_p, set);
}

static bool parse_h_char(const cel_char **src_p)
{
	return parse_not_char(src_p, '>') && parse_not(src_p, parse_newline)
	    && parse_dot(src_p);
}

static bool parse_q_char(const cel_char **src_p)
{
	return parse_not_char(src_p, '"') && parse_not(src_p, parse_newline)
	    && parse_dot(src_p);
}

static bool parse_header_name(const cel_char **src_p)
{
	const cel_char *src = *src_p;
	if (parse_char(&src, '<') && parse_plus(&src, parse_h_char)
	    && parse_char(&src, '>')) {
		*src_p = src;
		return true;
	}

	src = *src_p;
	if (parse_char(&src, '"') && parse_plus(&src, parse_q_char)
	    && parse_char(&src, '>')) {
		*src_p = src;
		return true;
	}
	return false;
}

static bool cel_pp_parse_token(const cel_char **src_p)
{
	return parse_newline(src_p) || parse_whitespace(src_p)
	    || parse_header_name(src_p) || parse_identifier(src_p)
	    || parse_pp_number(src_p) || parse_character_constant(src_p)
	    || parse_string_literal(src_p) || parse_punctuator(src_p)
	    || parse_dot(src_p);
}
