#ifndef __PRINT_H__
#define __PRINT_H__

#include "stdarg.h"  

int puts(const char *s);
int printf(const char *format, ...);
int vprintf(const char *format, va_list vlist);
int vsnprintf(char *buffer, int bufsz, const char *format, va_list vlist);
int snprintf(char *buffer, int bufsz, const char *format, ...);
#endif /* __PRINT_H__ */
