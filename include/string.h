#ifndef __STRING_H__
#define __STRING_H__

#include <bwio.h>

#define MIN(x,y) ((x) > (y) ? (y) : (x))
#define MAX(x,y) ((x) > (y) ? (x) : (y))

#define ABS(x) ((x) < 0 ? (x)*(-1) : (x))

int strlen(const char *str);
int strcmp(const char * str1, const char * str2);
int parse_token(char * str, char * token);
int a2i( char *src );
void sformat ( int *length, char *str, char *fmt, va_list va );
int sprintf( char *str, char *fmt, ... );
void * memset ( void * ptr, int value, int num );
void * memcpy ( void * destination, const void * source, int num );

#endif
