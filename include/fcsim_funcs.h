#ifndef __FCSIM_FUNCS_H__
#define __FCSIM_FUNCS_H__

#ifdef __cplusplus
extern "C" {
#endif

double fcsim_sin(double x);
double fcsim_cos(double x);
int fcsim_strtoi(const char *str, int len, int *res);
int fcsim_strtod(const char *str, int len, double *res);

#ifdef __cplusplus
}
#endif

#endif
