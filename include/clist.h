#ifndef __CLIST_H__
#define __CLIST_H__

struct clist {
	int * buf;

	int read;
	int write;
	int size;
};

void clist_init(struct clist * cl, int * buf, int size);
int clist_free(struct clist * cl);
int clist_len(struct clist * cl);
int clist_top(struct clist * cl, int * data);
int clist_pop(struct clist * cl, int * data);
int clist_push(struct clist * cl, int data);

#endif
