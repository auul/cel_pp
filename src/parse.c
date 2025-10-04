#include "parse.h"
#include "list.h"

#include <stdio.h>

#define PARSE_EVAL1(a) a
#define PARSE_EVAL2(a) PARSE_EVAL1(PARSE_EVAL1(a))
#define PARSE_EVAL4(a) PARSE_EVAL2(PARSE_EVAL2(a))
#define PARSE_EVAL8(a) PARSE_EVAL4(PARSE_EVAL4(a))
#define PARSE_EVAL(a) PARSE_EVAL8(PARSE_EVAL8(a))
#define PARSE_PARENS ()

#define parse_try(step, ...)                   \
	{                                          \
		const char *mark = *src_p;             \
		bool status = true;                    \
		status = parse_seq(step, __VA_ARGS__); \
		if (status) {                          \
			return true;                       \
		}                                      \
		*src_p = mark;                         \
	}

#define parse_seq(step, ...) step __VA_OPT__(; PARSE_SEQ_BEGIN(__VA_ARGS__))
#define PARSE_SEQ_BEGIN(step, ...)                \
	do {                                          \
		PARSE_EVAL(PARSE_SEQ(step, __VA_ARGS__)); \
	} while (0)
#define PARSE_SEQ(step, ...)                                              \
	if (!status) {                                                        \
		break;                                                            \
	}                                                                     \
	status = step __VA_OPT__(; PARSE_SEQ_AGAIN PARSE_PARENS(__VA_ARGS__))
#define PARSE_SEQ_AGAIN() PARSE_SEQ

#define parse_repeat(lo, hi, step, ...)            \
	status;                                        \
	do {                                           \
		const char *mark;                          \
		for (size_t i = 0; i < lo; i++) {          \
			mark = *src_p;                         \
			status = parse_seq(step, __VA_ARGS__); \
			if (!status || *src_p == mark) {       \
				status = false;                    \
				break;                             \
			}                                      \
		}                                          \
		if (!status) {                             \
			break;                                 \
		}                                          \
		for (size_t i = lo; i < hi; i++) {         \
			mark = *src_p;                         \
			status = parse_seq(step, __VA_ARGS__); \
			if (!status || *src_p == mark) {       \
				status = true;                     \
				*src_p = mark;                     \
				break;                             \
			}                                      \
		}                                          \
	} while (0)

#define parse_star(step, ...) parse_repeat(0, SIZE_MAX, step, __VA_ARGS__)
#define parse_plus(step, ...) parse_repeat(1, SIZE_MAX, step, __VA_ARGS__)

#define parse_effect(block) \
	status;                 \
	block

bool parse_char(const char **src_p, char c)
{
	const char *src = *src_p;
	if (*src == c) {
		*src_p = src + 1;
		return true;
	}
	return false;
}

bool parse_urange(const char **src_p, unsigned char lo, unsigned char hi)
{
	const char *src = *src_p;
	if ((unsigned char)*src >= lo && (unsigned char)*src <= hi) {
		*src_p = src + 1;
		return true;
	}
	return false;
}

bool parse_range(const char **src_p, char lo, char hi)
{
	return parse_urange(src_p, (unsigned char)lo, (unsigned char)hi);
}

bool parse_set(const char **src_p, const char *set)
{
	for (size_t i = 0; set[i]; i++) {
		if (parse_char(src_p, set[i])) {
			return true;
		}
	}
	return false;
}

bool parse_whitespace_opt(const char **src_p)
{
	parse_try(parse_plus(parse_set(src_p, " \f\n\r\t\v")));
	return true;
}

bool parse_digit(const char **src_p)
{
	return parse_range(src_p, '0', '9');
}

bool parse_nat(uint64_t *dest, const char **src_p)
{
	uint64_t prev = 0;
	*dest = 0;

	parse_try(
		parse_plus(
			parse_effect(prev = *dest; *dest = (*dest * 10) + **src_p - '0'),
			parse_digit(src_p)),
		parse_effect(*dest = prev));
	return false;
}

bool parse_bare_list(cel_pp_list **dest, const char **src_p)
{
	cel_pp_data data;
	parse_try(
		parse_effect(*dest = NULL), parse_whitespace_opt(src_p),
		parse_data(&data, src_p), parse_effect(list_append(&dest, data)),
		parse_star(
			parse_whitespace_opt(src_p), parse_char(src_p, ','),
			parse_whitespace_opt(src_p), parse_data(&data, src_p),
			parse_effect(list_append(&dest, data))),
		parse_whitespace_opt(src_p));
	list_unref(*dest);
	*dest = NULL;
	return parse_whitespace_opt(src_p);
}

bool parse_list(cel_pp_list **dest, const char **src_p)
{
	parse_try(
		parse_char(src_p, '['), parse_bare_list(dest, src_p),
		parse_char(src_p, ']'));
	return false;
}

bool parse_data(cel_pp_data *dest, const char **src_p)
{
	parse_try(
		parse_effect(dest->type = CEL_PP_DATA_NAT),
		parse_nat(&dest->nat, src_p));
	parse_try(
		parse_effect(dest->type = CEL_PP_DATA_LIST),
		parse_list(&dest->list, src_p));
	return false;
}
