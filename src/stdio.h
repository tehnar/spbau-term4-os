#ifndef __PRINT_H__
#define __PRINT_H__

#include "stdarg.h"  
#include "interrupt.h"

int puts(const char *s);
int printf(const char *format, ...);
int vprintf(const char *format, va_list vlist);
int vsnprintf(char *buffer, int bufsz, const char *format, va_list vlist);
int snprintf(char *buffer, int bufsz, const char *format, ...);
#define DBG_ASSERT(cond)				\
do {							\
	if (!(cond)) {					\
		interrupt_disable();			\
		printf("Condition %s failed", #cond);	\
		while (1);				\
	}						\
} while (0)

#endif /* __PRINT_H__ */
