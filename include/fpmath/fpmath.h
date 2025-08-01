#ifndef __FPMATH_H__
#define __FPMATH_H__

double fp_sin(double x);
double fp_cos(double x);
double fp_atan2(double y, double x);
int fp_strtod(const char *str, int len, double *res);

#endif
