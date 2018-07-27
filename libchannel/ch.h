/*
   Copyright (c) 2003, Paul Stephen Borile
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:
   1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright
   notice
   3. Neither the name of the Paul Stephen Borile nor the
   names of its contributors may be used to endorse or promote products
   derived from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY Paul Stephen Borile ''AS IS'' AND ANY
   EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL Paul Stephen Borile BE LIABLE FOR ANY
   DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
   ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


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


// typedef defining generic free function parameter in clean method
typedef int ch_opaque_delete_func (void *);

// error codes
#define CH_MAGIC_ID 0x0CCA
#define CH_OK 1
#define CH_BAD_HANDLE -1
#define CH_NO_MEMORY -2
#define CH_WRONG_ATTR -3
#define CH_WRONG_VALUE -4
#define CH_WRONG_PARAM -5
#define CH_GET_NODATA -6
#define CH_GET_ENDOFTRANSMISSION -7
#define CH_FREE_NOT_REQUESTED -8

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

void *ch_create(ch_h *ch, int datalen);
int ch_put(ch_h *ch, void *block);
int ch_put_head(ch_h *ch, void *block);
int ch_get(ch_h *ch, void *block);
int ch_peek(ch_h *ch, void *block);
int ch_setattr(ch_h *ch, int attr, int val);
int ch_getattr(ch_h *ch, int attr, int *val);
int ch_clean(ch_h *ch, ch_opaque_delete_func (*del_func));
int ch_destroy(ch_h *ch);

#ifdef __cplusplus
}
#endif

#endif
