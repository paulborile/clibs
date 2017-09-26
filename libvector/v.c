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


#include <stdio.h>
#include <stdlib.h>

#include "v.h"

v_h *v_create(v_h *vh, int dim)
{
    v_h *v = vh;

    // if passed handle is null allocate a newone
    if (v == NULL)
    {
        if (( v = malloc(sizeof(v_h))) == NULL )
        {
            return NULL;
        }
        v->allocated = 1;
    }
    else
    {
        v->allocated = 0;
    }

    v->capacity = VECTOR_INIT_CAPACITY;
    // specify preallocated size from outside
    if ( dim > VECTOR_INIT_CAPACITY )
    {
        v->capacity = dim;
    }
    v->total = 0;
    v->items = malloc(sizeof(void *) * v->capacity);
    return v;
}

int v_len(v_h *v)
{
    return v->total;
}

static void _v_resize(v_h *v, int capacity)
{
    #ifdef DEBUG_ON
    printf("vector_resize: %d to %d\n", v->capacity, capacity);
    #endif

    void **items = realloc(v->items, sizeof(void *) * capacity);
    if (items) {
        v->items = items;
        v->capacity = capacity;
    }
}

void v_add(v_h *v, void *item)
{
    if (v->capacity == v->total)
        _v_resize(v, v->capacity * 2);
    v->items[v->total++] = item;
}

void v_set(v_h *v, int index, void *item)
{
    if (index >= 0 && index < v->total)
        v->items[index] = item;
}

void *v_get(v_h *v, int index)
{
    if (index >= 0 && index < v->total)
        return v->items[index];
    return NULL;
}

void v_delete(v_h *v, int index)
{
    if (index < 0 || index >= v->total)
        return;

    v->items[index] = NULL;

    for (int i = index; i < v->total - 1; i++) {
        v->items[i] = v->items[i + 1];
        v->items[i + 1] = NULL;
    }

    v->total--;

    if (v->total > 0 && v->total == v->capacity / 4)
        _v_resize(v, v->capacity / 2);
}

void v_destroy(v_h *v)
{
    free(v->items);
    if (v->allocated)
    {
        free(v);
    }
}


#ifdef TEST

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "v.h"

int main(int argc, char **argv)
{
    v_h v;
    char s_numbers[100][16];

    v_create(&v, 100);

    // init numbers and add them
    for (int i=0; i<100; i++)
    {
        sprintf(s_numbers[i], "%d", i);
        // add them
        v_add(&v, (void *) &s_numbers[i]);
    }

    // now check them
    for (int i=0; i<100; i++)
    {
        // add them
        char *s = (char *) v_get(&v, i);
        if ( s != s_numbers[i] )
        {
            printf("Error : %s and %s not the same", s, s_numbers[i]);
        }
        assert(s == s_numbers[i]);
    }

    // now remove an items
    v_delete(&v, 2);

    char *s = v_get(&v, 2);

    assert(s != NULL);

    // delete moves all data down so now vector (2) points to 3
    if ( s != s_numbers[3] )
    {
        printf("Error : %s and %s\n", s, s_numbers[3]);
    }
}

#endif
