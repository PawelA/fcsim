#include <fpmath/fpmath.h>

double sin(double x)
{
	return fp_sin(x);
}

double cos(double x)
{
	return fp_cos(x);
}

double atan2(double y, double x)
{
	return fp_atan2(y, x);
}

double sqrt(double x);
double fabs(double x);

double fmax(double x, double y)
{
	return x > y ? x : y;
}

double fmin(double x, double y)
{
	return x < y ? x : y;
}

float sinf(float x)
{
	return fp_sin(x);
}

float cosf(float x)
{
	return fp_cos(x);
}

float fabsf(float x);

double copysign(double x, double y);

double rint(double x);
