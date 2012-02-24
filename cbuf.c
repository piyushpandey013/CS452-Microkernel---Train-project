#include <cbuf.h>

// initializes read/write pointers in the cbuf
void cbuf_init(struct cbuf * cb, char * buf, int size) {
	cb->buf = buf;
	cb->read = cb->write = 0;
	cb->size = size;
}

/*
 * size = 10
 * 123456789
 *         |
 */
// returns number of readable bytes in the cbuf
int cbuf_len(struct cbuf * cb) {
	if (cb->write >= cb->read) {
		return (cb->write - cb->read);
	} else {
		return (cb->size - cb->read) + (cb->write);
	}
}

// returns number of bytes available to write in the cbuf
int cbuf_free(struct cbuf * cb) {
	return (cb->size - 1) - cbuf_len(cb);
}

// reads the first `len` bytes of the cbuf (moves the read pointer)
int cbuf_read(struct cbuf * cb, char * buf, int len) {
	int i, buflen;
	buflen = cbuf_len(cb);
	for (i = 0; (buflen > 0) && (len > 0); --buflen, --len, ++i) {
		buf[i] = cb->buf[cb->read];

		++(cb->read);		
		if (cb->read == cb->size) {
			cb->read = 0;
		}

	}
	
	return i;
}

// gets the first `len` bytes from cbuf without moving the read pointer
int cbuf_peek(struct cbuf * cb, char * buf, int len) {
	int i, buflen;
	buflen = cbuf_len(cb);
	
	int read_save = cb->read;
	for (i = 0; (buflen > 0) && (len > 0); --buflen, --len, ++i) {
		buf[i] = cb->buf[cb->read];

		++(cb->read);		
		if (cb->read == cb->size) {
			cb->read = 0;
		}

	}
	cb->read = read_save;
	
	return i;
}

// writes to cbuf and returns number of bytes written
int cbuf_write(struct cbuf * cb, const char * buf, int len) {
	int i, buflen;
	buflen = cbuf_len(cb);
	
	for (i = 0; (len > 0) && (cb->size - buflen > 1); --len, ++buflen, ++i) {
		cb->buf[cb->write] = buf[i];
		++(cb->write);
		
		if (cb->write == cb->size) {
			cb->write = 0;
		}
	}
	
	return i;
}
