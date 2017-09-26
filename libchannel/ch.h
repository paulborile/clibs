
#ifndef CH_H
#define CH_H

#ifdef __cplusplus
extern "C" {
#endif

struct _ch_elem_t
{
    void *block;
    struct _ch_elem_t *prev;
};
typedef struct _ch_elem_t ch_elem_t;

struct _ch_h
{
    short magic;
    short allocated;
    int datalen;
    int count;
    int attr;
    ch_elem_t *head;
    ch_elem_t *tail;
    pthread_mutex_t ch_mutex;
    pthread_cond_t ch_condvar;
    int waiting_threads;
    void (*free_fun)(); // USed ??
};
typedef struct _ch_h ch_h;

// error codes
#define CH_MAGIC_ID 0x0CCA
#define CH_OK 1
#define CH_BAD_HANDLE -1
#define CH_NO_MEMORY -2
#define CH_WRONG_ATTR -3
#define CH_WRONG_VALUE -4

// ch_create datalen types
#define CH_DATALEN_STRING -1
#define CH_DATALEN_VOIDP 0

// attribute names
#define CH_BLOCKING_MODE 100
#define CH_COUNT 200
// attributes values
#define CH_ATTR_NON_BLOCKING_GET 0
#define CH_ATTR_BLOCKING_GET 1

#define CH_ENDOFTRANSMISSION  ((void *)0xE)

// methods

void *ch_create(int datalen);
int ch_put(ch_h *ch, void *block);
int ch_put_head(ch_h *ch, void *block);
void *ch_get(ch_h *ch, void *block);
void *ch_peek(ch_h *ch, void *block);
int ch_setattr(ch_h *ch, int attr, int val);
int ch_getattr(ch_h *ch, int attr, int *val);
int ch_clean(ch_h *ch);
int ch_destroy(ch_h *ch);

#ifdef __cplusplus
}
#endif

#endif
