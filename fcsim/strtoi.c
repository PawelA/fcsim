#include <fcsim_funcs.h>

int fcsim_strtoi(const char *str, int len, int *res)
{
	int val = 0;
	int neg = 0;
	int digit;
	int i;

	if (len == 0)
		return -1;
	if (*str == '+' || *str == '-') {
		if (*str == '-')
			neg = 1;
		str++;
		len--;
	}
	for (i = 0; i < len; i++) {
		digit = str[i] - '0';
		if (digit < 0 || digit > 10)
			return -1;
		val = val * 10 + digit;
	}
	*res = neg ? -val : val;

	return 0;
}
