#ifndef __IO_H__
#define __IO_H__

typedef char *va_list;

#define __va_argsiz(t)	\
		(((sizeof(t) + sizeof(int) - 1) / sizeof(int)) * sizeof(int))

#define va_start(ap, pN) ((ap) = ((va_list) __builtin_next_arg(pN)))

#define va_end(ap)	((void)0)

#define va_arg(ap, t)	\
		 (((ap) = (ap) + __va_argsiz(t)), *((t*) (void*) ((ap) - __va_argsiz(t))))

#define COM1	0
#define COM2	1

void Printf(char *fmt, ...);

#ifdef DEBUG
#define Debug(fmt, ...) Printf("[Debug] " fmt, __VA_ARGS__)
#else
#define Debug(fmt) ((void*)0)
#define Debug(fmt, ...) ((void*)0)
#endif

#endif
