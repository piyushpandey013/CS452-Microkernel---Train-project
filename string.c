#include <string.h>

void * memcpy ( void * destination, const void * source, int num ) {
	char *csrc = (char*)source;
	char *cdest = (char*)destination;
	
	int i;
	for (i = 0; i < num; ++i) {
		*cdest++ = *csrc++;
	}
	
	return destination;
}

void * memset ( void * ptr, int value, int num ) {
	void * save = ptr;
	
	char *bytes = (char*)ptr;
	int i;
	for (i = 0; i < num; i++) {
		bytes[i] = value;
	}
	
	return save;
}

int strcmp(const char * str1, const char * str2) {
	while ((*str1 && *str2) && (*str1++ == *str2++));
	return !((*str1 == *str2) && (*--str1 == *--str2));
}

int strlen(const char *str) {
        int len;
        for (len = 0; *str++ != '\0'; ++len);
        
        return len;
}

int parse_token(char * str, char * token) {
	int count = 0;
	// skip whitespaces
	while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r') {
		++str;
		++count;
	}
	
	// read a word into 'token'
	for (; *str; *token++ = *str++) {
		if (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r')
			break;
		
		count++;
	}
	
	*token = '\0';

	// return # of characters processed till the end of this word
	return count;
}

int a2i( char *src ) {
	int num, digit;
	char *p;

	p = src; num = 0;
	char ch = '0';
	while( ( digit = (ch-'0') ) >= 0 && digit <= 9 ) {
		num = num*10 + digit;
		ch = *p++;
	}
	return num;
}


int sputc( char **str, char c ) {
	*((*str)++) = c;
	return 0;
}

int sputx( char **str, char c ) {
	char chh, chl;

	chh = c2x( c / 16 );
	chl = c2x( c % 16 );
	sputc( str, chh );
	return sputc( str, chl );
}

int sputr( char **str, unsigned int reg ) {
	int byte;
	char *ch = (char *) &reg;

	for( byte = 3; byte >= 0; byte-- ) sputx( str, ch[byte] );
	return sputc( str, ' ' );
}

int sputstr( char **target, char *str ) {
	while( *str ) {
		if( sputc( target, *str ) < 0 ) return -1;
		str++;
	}
	return 0;
}

void sputw( char **str, int n, char fc, char *bf ) {
	char ch;
	char *p = bf;

	while( *p++ && n > 0 ) n--;
	while( n-- > 0 ) sputc( str, fc );
	while( ( ch = *bf++ ) ) sputc( str, ch );
}

void sformat ( int *length, char *str, char *fmt, va_list va ) {
	char bf[12];
	char ch, lz;
	int w;

	char *start = str;
	
	while ( ( ch = *(fmt++) ) ) {
		if ( ch != '%' )
			sputc( &str, ch );
		else {
			lz = 0; w = 0;
			ch = *(fmt++);
			switch ( ch ) {
			case '0':
				lz = 1; ch = *(fmt++);
				break;
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				ch = bwa2i( ch, &fmt, 10, &w );
				break;
			}
			switch( ch ) {
			case 0: return;
			case 'c':
				sputc( &str, va_arg( va, char ) );
				break;
			case 's':
				sputw( &str, w, 0, va_arg( va, char* ) );
				break;
			case 'u':
				bwui2a( va_arg( va, unsigned int ), 10, bf );
				sputw( &str, w, lz, bf );
				break;
			case 'd':
				bwi2a( va_arg( va, int ), bf );
				sputw( &str, w, lz, bf );
				break;
			case 'x':
				bwui2a( va_arg( va, unsigned int ), 16, bf );
				sputw( &str, w, lz, bf );
				break;
			case 'b':
				bwui2a( va_arg( va, unsigned int ), 2, bf );
				sputw( &str, w, lz, bf );
				break;
			case '%':
				sputc( &str, ch );
				break;
			}
		}
	}
	*str = 0;
	*length = (str - start);
}

int sprintf( char *str, char *fmt, ... ) {
        
    int length = 0;

    va_list va;

    va_start(va,fmt);
    sformat( &length, str, fmt, va );
    va_end(va);

    return length;
}
