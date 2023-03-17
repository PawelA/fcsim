#define PI 3.1415926535

double sin(double x)
{
	return x - x*x*x / 6 + x*x*x*x*x / 120;
}

double cos(double x)
{
	return 1.0 - x*x / 2 + x*x*x*x / 24;
}

double atan2(double y, double x)
{
	double yx;
	double a;

	if (x == 0) {
		if (y >= 0)
			return PI / 2;
		else
			return -PI / 2;
	}

	yx = y / x;
	a = yx - yx*yx*yx / 3 + yx*yx*yx*yx*yx / 5;

	if (x > 0)
		return a;
	else if (y >= 0)
		return a + PI;
	else
		return a - PI;

}

double sqrt(double x)
{
	return __builtin_sqrt(x);
}

double fabs(double x)
{
	return x > 0 ? x : -x;
}

