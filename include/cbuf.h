#ifndef __CBUF_H__
#define __CBUF_H__

struct cbuf {
	char * buf;
	int read;
	int write;
	int size;
};

void cbuf_init(struct cbuf * cb, char * buf, int size);
int cbuf_free(struct cbuf * cb);
int cbuf_len(struct cbuf * cb);
int cbuf_peek(struct cbuf * cb, char * buf, int len);
int cbuf_read(struct cbuf * cb, char * buf, int len);
int cbuf_write(struct cbuf * cb, const char * buf, int len);

#endif
