#include <clist.h>

// initializes read/write pointers in the cbuf
void clist_init(struct clist * cl, int * buf, int size) {
	cl->buf = buf;
	cl->read = cl->write = 0;
	cl->size = size;
}

// returns number of readable bytes in the cbuf
int clist_len(struct clist * cl) {
	if (cl->write >= cl->read) {
		return (cl->write - cl->read);
	} else {
		return (cl->size - cl->read) + (cl->write);
	}
}

// returns number of bytes available to write in the cbuf
int clist_free(struct clist * cl) {
	return (cl->size - 1) - clist_len(cl);
}

// reads the first `len` bytes of the cbuf (moves the read pointer)
int clist_pop(struct clist * cl, int *data) {

	if (clist_len(cl) > 0) {
		*data = cl->buf[cl->read];
		cl->read = (cl->read + 1) % cl->size;
	} else {
		return -1; // nothing to pop
	}
	
	return 1;
}

// gets the first `len` bytes from cbuf without moving the read pointer
int clist_top(struct clist * cl, int *data) {

	if (clist_len(cl) > 0) {
		*data = cl->buf[cl->read];
	} else {
		return -1; // nothing to pop
	}
	
	return 1;
}

// writes to cbuf and returns number of bytes written
int clist_push(struct clist * cl, int data) {

	if (clist_free(cl) > 0) {
		cl->buf[cl->write] = data;
		cl->write = (cl->write + 1) % cl->size;
	} else {
		return -1; // not enough space to write
	}

	return 1;
}
