/*
   Copyright (c) 2003, Paul Stephen Borile
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:
   1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
   3. All advertising materials mentioning features or use of this software
   must display the following acknowledgement:
   This product includes software developed by the <organization>.
   4. Neither the name of the <organization> nor the
   names of its contributors may be used to endorse or promote products
   derived from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDER> ''AS IS'' AND ANY
   EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
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
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <assert.h>

#include    "fh.h"

#define FH_ALLHSLOT(a)   a = (fh_slot *)malloc( sizeof( fh_slot ) ); \
    if(a != NULL) \
    {  \
        a->key = NULL;    \
        a->next = NULL;    \
        a->opaque_obj = NULL;   \
    }
#define FH_ALLOPQOBJ(a,b)    a = (void *) malloc(b);

/*
 * using default hash found in cfu_hash (the perl one)
 */

static unsigned int fh_default_hash(const char *key, int dim) {
    register size_t i = strlen(key);
    register unsigned int hv = 0; // could put a seed here instead of zero
    register const unsigned char *s = (unsigned char *)key;
    while (i--) {
        hv += *s++;
        hv += (hv << 10);
        hv ^= (hv >> 6);
    }
    hv += (hv << 3);
    hv ^= (hv >> 11);
    hv += (hv << 15);

    return hv & (dim - 1);
}

/* makes sure the real size of the buckets array is a power of 2 */
static unsigned int fh_hash_size(unsigned int s) {
    unsigned int i = 1;
    while (i < s) i <<= 1;
    return i;
}

// create hashtable object and init all data
fh_t *fh_create(int dim, int datalen, unsigned int (*hash_function)())
{
    fh_t *f = NULL;
    f_hash *hash = NULL;
    int real_dim = fh_hash_size(dim);

    // allocates the hash table
    hash = (f_hash *) calloc(real_dim, sizeof(f_hash));
    if (hash == NULL)
    {
        return (NULL);
    }

    // hash handle to return

    f = (fh_t *) malloc(sizeof(fh_t));
    if (f == NULL)
    {
        free(hash);
        return (NULL);
    }

    // init values

    f->h_dim = real_dim;
    f->h_datalen = datalen;
    f->h_elements = 0;
    f->h_collision = 0;
    if (hash_function != NULL)
    {
        f->hash_function = hash_function;
    }
    else
    {
        f->hash_function = fh_default_hash;
    }

    f->hash_table = hash;

    return (f);
}

// set attributes in object
int fh_setattr(fh_t *fh, int attr, int value)
{
    return (1);
}

// get attributes in object
int fh_getattr(fh_t *fh, int attr, int *value)
{
    switch(attr)
    {
    case FH_ATTR_ELEMENT:
        (*value) = fh->h_elements;
        break;
    case FH_ATTR_DIM:
        (*value) = fh->h_dim;
        break;
    case FH_ATTR_COLLISION:
        (*value) = fh->h_collision;
        break;
    default:
        return(FH_BAD_ATTR);
    }
    return (1);
}


// old hash function
static unsigned int fh_default_hash_orig(char *name, int i)
{
    unsigned long h = 0, g;
    while (*name)
    {
        h = (h << 4) + *name++;
        g = (h & 0xF0000000);
        if (g)
            h ^= g >> 24;
        h &= ~g;
    }
    return h % i;
}

/*
 * deallocates all data and destroys 'da hash
 */

int fh_destroy(fh_t *fh)
{
    int i;
    register fh_slot *h_slot = NULL, *h_slot_current = NULL;
    f_hash *f_h = (f_hash *) fh->hash_table;

    // dealloco tutti gli slot della hash
    for (i = 0; i < fh->h_dim; i++)
    {

        h_slot = f_h[i].h_slot;

        while (h_slot != NULL)
        {
            // elimino lo slot

            h_slot_current = h_slot;
            h_slot = h_slot->next;

            //
            // free dell'oggetto opaco e della struttura h_slot

            // l'opaco lo libero solo se != NULL
            if ( h_slot_current->opaque_obj != NULL )
                free(h_slot_current->opaque_obj);

            free(h_slot_current->key);
            free(h_slot_current);
        }
    }

    // dealloco hash table
    free(fh->hash_table);

    // dealloco struct hash info
    free(fh);

    return (1);
}

// insert : copies both key and opaque data. If slot already used
// allocates a newone and incremets h_collision
int fh_insert(fh_t *fh, char *key, void *block)
{
    int i;
    register fh_slot *h_slot, *new_h_slot;
    void *new_opaque_obj = NULL;

    // looking for slot

    i = fh->hash_function(key, fh->h_dim);

    assert(i<fh->h_dim);

    h_slot = fh->hash_table[i].h_slot;

    if (h_slot != NULL)
    {
        (fh->h_collision)++;
    }

    //  scan list to end while checking for duplicates

    while (h_slot != NULL)
    {
        if ( strcmp(h_slot->key, key) == 0 )
        {
            // we have a duplicate key
            return (FH_DUPLICATED_ELEMENT);
        }
        h_slot = h_slot->next;
    }

    // new slot to add
    FH_ALLHSLOT(new_h_slot);

    if (new_h_slot == NULL)
    {
        return (FH_NO_MEMORY);
    }

    // allocate and copy key
    int key_len = strlen(key);
    new_h_slot->key = (char *) malloc(key_len + 1);
    if (new_h_slot->key == NULL)
    {
        free(new_h_slot);
        return (FH_NO_MEMORY);
    }

    strcpy(new_h_slot->key, key);

    // allocate and copy opaque object

    if ( block != NULL )
    {
        // datalen contains a positive value = fixed size opaque_obj
        if ( fh->h_datalen > 0 )
        {
            FH_ALLOPQOBJ(new_opaque_obj, fh->h_datalen);
            if (new_opaque_obj == NULL)
            {
                free(new_h_slot->key);
                free(new_h_slot);

                return (FH_NO_MEMORY);
            }
            memcpy(new_opaque_obj, block, fh->h_datalen);
        }
        else if ( fh->h_datalen == 0 ) // datalen = 0 just copy pointers
        {
            new_opaque_obj = block;
        }
        else if ( fh->h_datalen == -1 ) // datalen = -1 => opaque is string
        {
            int len = strlen(block);
            FH_ALLOPQOBJ(new_opaque_obj, len + 1);
            strcpy(new_opaque_obj, block);
        }

        new_h_slot->opaque_obj = new_opaque_obj;
    }

    fh_slot *old_first_h_slot = fh->hash_table[i].h_slot;

    fh->hash_table[i].h_slot = new_h_slot;
    new_h_slot->next = old_first_h_slot;

    (fh->h_elements)++;

    return (i);
}

// fh_del - remove item from hash and free memory
int fh_del(fh_t *fh, char *key)
{
    int i;
    register fh_slot *h_slot, *prev_h_slot = NULL;

    // looking for slot

    i = fh->hash_function(key, fh->h_dim);

    assert(i<fh->h_dim);

    h_slot = fh->hash_table[i].h_slot;

    // scan until end or element found

    while ((h_slot != NULL) && (strcmp(h_slot->key, key)))
    {
        prev_h_slot = h_slot;
        h_slot = h_slot->next;
    }

    if ( h_slot == NULL )
    {
        return (FH_ELEMENT_NOT_FOUND);
    }

    // remove the slot

    if (prev_h_slot != NULL)
    {
        prev_h_slot->next = h_slot->next;
    }
    else // slot to delete is first in list
    {
        fh->hash_table[i].h_slot = h_slot->next;
    }

    // cleanup only for fixed size of string opaque object
    if ((h_slot->opaque_obj) && (fh->h_datalen != 0))
        free(h_slot->opaque_obj);

    free(h_slot->key);
    free(h_slot);
    h_slot = NULL;

    (fh->h_elements)--;

    return (i);
}

// serch key and copy out opaque data
int fh_search(fh_t *fh, char *key, void *block)
{
    int i;
    register fh_slot *h_slot;

    i = fh->hash_function(key, fh->h_dim);

    assert(i<fh->h_dim);

    h_slot = fh->hash_table[i].h_slot;

    while ((h_slot != NULL) && (strcmp(h_slot->key, key)))
    {
        h_slot = h_slot->next;
    }

    if ( h_slot == NULL )
    {
        return (FH_ELEMENT_NOT_FOUND);
    }

    // copy out
    if (h_slot->opaque_obj)
    {
        if ( fh->h_datalen > 0 ) // fixed size opaque_obj
        {
            memcpy(block, h_slot->opaque_obj, fh->h_datalen);
        }
        else if ( fh->h_datalen == -1 ) // copy string
        {
            strcpy(block, h_slot->opaque_obj);
        }
        else
        {
            block = h_slot->opaque_obj;
        }
    }

    return (i);
}

// search and return locked pointer to opaque
// allows modifying an opaque object entry without del/insert TODO modify when lock are introduced
void *fh_searchlock(fh_t *fh, char *key, int *slot)
{
    int i;
    register fh_slot *h_slot;

    i = fh->hash_function(key, fh->h_dim);

    assert(i<fh->h_dim);

    h_slot = fh->hash_table[i].h_slot;

    while ((h_slot != NULL) && (strcmp(h_slot->key, key)))
    {
        h_slot = h_slot->next;
    }

    if ( h_slot == NULL )
    {
        *slot = FH_ELEMENT_NOT_FOUND;
        return (NULL);
    }
    (*slot) = i;
    return (h_slot->opaque_obj);
}

// start scan and find first full entry in hash returning index and pointer to slot
// call with *index containing 0 to start from beginning
int fh_scan_start(fh_t *fh, int start_index, void **slot)
{
    int i;
    for (i = start_index; i < fh->h_dim; i++)
    {
        if (fh->hash_table[i].h_slot != NULL)
        {
            *slot = fh->hash_table[i].h_slot;
            return(i);
        }
    }
    // empty hashtable or start_index points to another index in the hashtable and from that point on no data is present.
    *slot = NULL;
    return (FH_ELEMENT_NOT_FOUND);
}

// takes index and slot as point of last scan and starting from there returns next entry (copies key and opaque)
// if index and slot are not valida anymore return FH_ELEMENT_NOT_FOUND but continues
int fh_scan_next(fh_t *fh, int *index, void **slot, char *key, void *block)
{
    int i, el_not_found = 0;
    register fh_slot *h_slot;

    h_slot = fh->hash_table[(*index)].h_slot;
    while ((h_slot != NULL) && (h_slot != (fh_slot *)(*slot) ))
    {
        h_slot = h_slot->next;
    }

    if (h_slot == NULL)
    {
        // slot not found, find next but return error, another thread may have cancelled the element.
        el_not_found = 1;

        for (i = (*index) + 1; i < fh->h_dim; i++)
        {

            if (fh->hash_table[i].h_slot != NULL)
            {
                (*index) = i;
                *slot = fh->hash_table[i].h_slot;

                return (FH_ELEMENT_NOT_FOUND);
            }

        }
    }
    else
    {
        // slot found return data
        strcpy(key, h_slot->key);
        // copy out
        if (h_slot->opaque_obj)
        {
            if ( fh->h_datalen > 0 ) // fixed size opaque_obj
            {
                memcpy(block, h_slot->opaque_obj, fh->h_datalen);
            }
            else if ( fh->h_datalen == -1 ) // copy string
            {
                strcpy(block, h_slot->opaque_obj);
            }
            else
            {
                block = h_slot->opaque_obj;
            }
        }

        // return next slot

        if (h_slot->next != NULL)
        {
            // same index
            *slot = h_slot->next;
            return (1);
        }


        for (i = (*index) + 1; i < fh->h_dim; i++)
        {
            // finding next full h_slot

            if (fh->hash_table[i].h_slot != NULL)
            {
                *slot = fh->hash_table[i].h_slot;
                (*index) = i;

                return (1);
            }
        }
    }

    // end of hash
    *slot = NULL;

    if (el_not_found == 1)
    {
        return (FH_SCAN_END);
    }
    else
    {
        return (1);
    }
}
