#ifndef __BWIO_H__
#define __BWIO_H__
/*
 * bwio.h
 */

#include <io.h>

#define ON	1
#define	OFF	0

int bwsetfifo( int channel, int state );

int bwsetspeed( int channel, int speed );

int bwputc( int channel, char c );

int bwgetc( int channel );

int bwputx( int channel, char c );

int bwputstr( int channel, char *str );

int bwputr( int channel, unsigned int reg );

void bwputw( int channel, int n, char fc, char *bf );

void bwprintf( int channel, char *format, ... );

char bwa2i( char ch, char **src, int base, int *nump );

char c2x( char ch );
void bwui2a( unsigned int num, unsigned int base, char *bf );
void bwi2a( int num, char *bf );

#endif
