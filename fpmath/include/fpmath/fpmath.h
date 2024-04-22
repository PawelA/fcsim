#ifndef __FPMATH_H__
#define __FPMATH_H__

#ifdef __cplusplus
extern "C" {
#endif

double fp_sin(double x);
double fp_cos(double x);
double fp_atan2(double y, double x);
int fp_strtod(const char *str, int len, double *res);

#ifdef __cplusplus
}
#endif

#endif
