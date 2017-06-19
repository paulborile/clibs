/*
   Copyright (c) 2017, Paul Stephen Borile
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
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

#include "fh.h"
#include "lru.h"


// lru_create : create lru object
// lru is basically an hashtable with a maximum number of elements that can be added. When the lru is full,
// the least recently used entry is removed to make space for the new one.
// It is based on a libfh hastable and on a libll linked list
// key is always a string (char *, '\0' terminated), while datalen is always a void *

lru_t *lru_create(int dim)
{
    lru_t *l = NULL;
    // allocate lru_t
    l = (lru_t *) malloc(sizeof(lru_t));
    if (l == NULL)
    {
        return (NULL);
    }

    l->lru_magic = LRU_MAGIC_ID;
    l->lru_size = dim; // maximum number of objects to kept in hash

    if (( l->fh = fh_create(dim, FH_DATALEN_VOIDP, NULL)) == NULL )
    {
        return (NULL);
    }

    // l->ll = ll_create(dim)) == NULL )

    return(l);
}

int     lru_add(lru_t *lru, char *key, void *payload)
{
    return(1);
}

int     lru_check(lru_t *lru, char *key, void **payload )
{
    return(1);
}

int     lru_destroy(lru_t *lru )
{
    return(1);
}

/*
   int     lru_setattr(lru_t *lru, int attr, int value)
   {

   }
   int     lru_getattr(lru_t *lru, int attr, int *value)
   {

   }
 */