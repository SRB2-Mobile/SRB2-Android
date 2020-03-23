// Lactozilla: Re-implementation of localeconv
#include <limits.h>

struct lconv
{
	char *decimal_point;
	char *thousands_sep;
	char *grouping;
	char *int_curr_symbol;
	char *currency_symbol;
	char *mon_decimal_point;
	char *mon_thousands_sep;
	char *mon_grouping;
	char *positive_sign;
	char *negative_sign;
	char int_frac_digits;
	char frac_digits;
	char p_cs_precedes;
	char p_sep_by_space;
	char n_cs_precedes;
	char n_sep_by_space;
	char p_sign_posn;
	char n_sign_posn;
	char int_p_cs_precedes;
	char int_p_sep_by_space;
	char int_n_cs_precedes;
	char int_n_sep_by_space;
	char int_p_sign_posn;
	char int_n_sign_posn;
};

struct lconv *localeconv(void)
{
	static struct lconv C_LCONV[1] = {
		{
			.decimal_point = ".",
			.thousands_sep = "",
			.grouping = "",
			.int_curr_symbol = "",
			.currency_symbol = "",
			.mon_decimal_point = "",
			.mon_thousands_sep = "",
			.mon_grouping = "",
			.positive_sign = "",
			.negative_sign = "",
			.int_frac_digits = CHAR_MAX,
			.frac_digits = CHAR_MAX,
			.p_cs_precedes = CHAR_MAX,
			.p_sep_by_space = CHAR_MAX,
			.n_cs_precedes = CHAR_MAX,
			.n_sep_by_space = CHAR_MAX,
			.p_sign_posn = CHAR_MAX,
			.n_sign_posn = CHAR_MAX,
			.int_p_cs_precedes = CHAR_MAX,
			.int_p_sep_by_space = CHAR_MAX,
			.int_n_cs_precedes = CHAR_MAX,
			.int_n_sep_by_space = CHAR_MAX,
			.int_p_sign_posn = CHAR_MAX,
			.int_n_sign_posn = CHAR_MAX,
		}
	};
	return C_LCONV;
}
