int parse_double(char *str, double *rres)
{
	double num = 0.0;
	double den = 1.0;
	double e = 1.0;
	char c;

	if (*str == '-') {
		str++;
		den = -den;
	}

	for (; *str; str++) {
		c = *str;
		if (c == '.') {
			e = 10.0;
		} else if ('0' <= c && c <= '9') {
			num = num * 10.0 + (c - '0');
			den = den * e;
		} else {
			return 0;
		}
	}
	*rres = num / den;

	return 1;
}
