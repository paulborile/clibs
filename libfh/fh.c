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

// WARNING!!!! Needed to use strdup in this code. Without it, it's not defined
#define _BSD_SOURCE

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
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

#define FH_CHECK(f) if ((!f) || (f->h_magic != FH_MAGIC_ID)) return (FH_BAD_HANDLE);

#define FH_KEY_CHECK(key) if (!key || key[0] == '\0') return (FH_INVALID_KEY);

#define FHE_CHECK(f) if ((!f) || (f->magic != FHE_MAGIC_ID)) return (FH_BAD_HANDLE);

/*
 * oat hash (one at a time hash), Bob Jenkins, used by cfu hash and perl
 */

static unsigned int fh_default_hash(const char *key, int dim)
{
    register unsigned int hv = 0; // could put a seed here instead of zero
    register const unsigned char *s = (unsigned char *)key;
    while (*s) {
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
static unsigned int fh_hash_size(unsigned int s)
{
    unsigned int i = 1;
    while (i < s) i <<= 1;
    return i;
}

// old hash function
/*
   static unsigned int fh_default_hash_orig(const char *key, int dim)
   {
    unsigned long h = 0, g;
    while (*key)
    {
        h = (h << 4) + *key++;
        g = (h & 0xF0000000);
        if (g)
            h ^= g >> 24;
        h &= ~g;
    }
    return h % dim;
   }
 */

// Compare functions to sort enumerator
int fh_ascfunc(const void *a, const void *b)
{
    return ( strcmp(((fh_elem_t *)a)->key, ((fh_elem_t *)b)->key) );
}

int fh_descfunc(const void *a, const void *b)
{
    return ( strcmp(((fh_elem_t *)b)->key, ((fh_elem_t *)a)->key) );
}

// create hashtable object and init all data
// datalen = -1 : opaque is a string so it spave for it will be allocated with malloc
// and it will be copied with strcpy.
// datalen > 0 : opaque is a fixed size data structure (struct?); will be allocated with malloc
// and copied with memcpy
// datalen = 0 : opaque is threated as a void pointer, no allocation is done, pointer value is kept in hashtable opa	que data.

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

    // init elements/collisions critical region mutex
    pthread_mutex_init(&(f->fh_lock), NULL);

    f->h_elements = 0;
    f->h_collision = 0;

    f->h_magic = FH_MAGIC_ID;
    f->h_attr = 0;

    if (hash_function != NULL)
    {
        f->hash_function = hash_function;
    }
    else
    {
        f->hash_function = fh_default_hash;
    }

    // compute pool size (function of hashtable size), save size in fh object, allocate lock pool, init locks
    if(dim > SIZE_LIMIT_SINGLE_MUTEX)
    {
        // Compute mutex number: it's base 2 logarythm of hashtable dimension
        int num = dim;
        int size = 0;

        while(num > 0)
        {
            int half = num >> 1;
            size++;
            num = half;
        }

        f->n_lock = size;
    }
    else
    {
        // small hashtable, create only one mutex
        f->n_lock = 1;
    }

    f->h_lock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t) * f->n_lock);
    if(f->h_lock == NULL)
    {
        free(hash);
        free(f);
        return NULL;
    }

    // init hastable critical region mutexes
    for (int i = 0; i < f->n_lock; i++)
    {
        pthread_mutex_init(&(f->h_lock[i]), NULL);
    }

    f->hash_table = hash;

    return (f);
}

static void _fh_lock(fh_t *fh, int slot)
{
    pthread_mutex_lock(&(fh->h_lock[slot % fh->n_lock]));
}

static void _fh_unlock(fh_t *fh, int slot)
{
    pthread_mutex_unlock(&(fh->h_lock[slot % fh->n_lock]));
}

static void _fh_lock_fh(fh_t *fh)
{
    pthread_mutex_lock(&(fh->fh_lock));
}

static void _fh_unlock_fh(fh_t *fh)
{
    pthread_mutex_unlock(&(fh->fh_lock));
}

//static void _fh_lock_all(fh_t *fh)
//{
//    for (int i = 0; i < fh->n_lock; i++)
//    {
//        pthread_mutex_lock(&(fh->h_lock[i]));
//    }
//}

//static void _fh_unlock_all(fh_t *fh)
//{
//    for (int i = 0; i < fh->n_lock; i++)
//    {
//        pthread_mutex_unlock(&(fh->h_lock[i]));
//    }
//}

// set attributes in object
int fh_setattr(fh_t *fh, int attr, int value)
{
    (void) value;
    FH_CHECK(fh);

    switch(attr)
    {
    case FH_SETATTR_DONTCOPYKEY:
        fh->h_attr |= FH_SETATTR_DONTCOPYKEY;
        break;
    default:
        return(FH_BAD_ATTR);
    }

    return (FH_OK);
}

// get attributes in object
int fh_getattr(fh_t *fh, int attr, int *value)
{
    FH_CHECK(fh);

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
    return (FH_OK);
}

// Clean all elements in hash table, leaving the table empty
int fh_clean(fh_t *fh, fh_opaque_delete_func (*del_func))
{
    FH_CHECK(fh);
    int result = FH_OK;

    // User set del_func but hash table doesn't contain void pointers: set error to return (but still clean the table)
    if(del_func != NULL && fh->h_datalen != FH_DATALEN_VOIDP)
    {
        result = FH_FREE_NOT_REQUESTED;
    }

    fh_enum_t *fhe = fh_enum_create(fh, 0, &result);

    // If enumerator is NULL something goes wrong. Return error to caller.
    if(fhe == NULL)
    {
        return result;
    }

    while ( fh_enum_is_valid(fhe) )
    {
        fh_elem_t *element = fh_enum_get_value(fhe, &result);

        // free opaque object only if was allocated
        if (fh->h_datalen == FH_DATALEN_VOIDP)
        {
            // If delete object function is passed, use it to delete opaque. Otherwise do nothing
            if(del_func != NULL)
            {
                del_func(element->opaque_obj);
            }
        }

        fh_del(fh, element->key);

        // Move to next element
        fh_enum_move_next(fhe);
    }

    // Destroy the enum
    fh_enum_destroy(fhe);

    return(result);
}

// deallocates all data (using fh_clean) and destroys 'da hash
int fh_destroy(fh_t *fh)
{
    FH_CHECK(fh);

    fh_clean(fh, NULL);
    // remove all entries

    // dealloco hash table
    free(fh->hash_table);

    // free every single mutex
    for (int i = 0; i < fh->n_lock; i++)
    {
        pthread_mutex_destroy(&fh->h_lock[i]);
    }

    // Free mutex pool
    free(fh->h_lock);

    pthread_mutex_destroy(&fh->fh_lock);

    // dealloco struct hash info
    free(fh);

    return (FH_OK);
}

// insert : copies both key and opaque data. If slot already used
// allocates a newone and incremets h_collision
int fh_insert(fh_t *fh, char *key, void *block)
{
    int i;
    fh_slot *h_slot, *new_h_slot = NULL;
    void *new_opaque_obj = NULL;
    FH_CHECK(fh);
    FH_KEY_CHECK(key);

    // looking for slot

    i = fh->hash_function(key, fh->h_dim);
    _fh_lock(fh, i);

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
            _fh_unlock(fh, i);
            return (FH_DUPLICATED_ELEMENT);
        }
        h_slot = h_slot->next;
    }

    // new slot to add
    new_h_slot = calloc(1, sizeof(fh_slot));
    if (new_h_slot == NULL)
    {
        _fh_unlock(fh, i);
        return (FH_NO_MEMORY);
    }

    // allocate and copy key
    if ( fh->h_attr & FH_SETATTR_DONTCOPYKEY )
    {
        new_h_slot->key = key;
    }
    else
    {
        new_h_slot->key = strdup(key);
        if (new_h_slot->key == NULL)
        {
            free(new_h_slot);
            _fh_unlock(fh, i);
            return (FH_NO_MEMORY);
        }
    }

    // allocate and copy opaque object

    if ( block != NULL )
    {
        // datalen contains a positive value = fixed size opaque_obj
        if ( fh->h_datalen > 0 )
        {
            new_opaque_obj = malloc(fh->h_datalen);
            if (new_opaque_obj == NULL)
            {
                _fh_unlock(fh, i);
                return (FH_NO_MEMORY);
            }
            memcpy(new_opaque_obj, block, fh->h_datalen);
        }
        else if ( fh->h_datalen == FH_DATALEN_VOIDP ) // datalen 0 means just copy opaque pointers
        {
            new_opaque_obj = block;
        }
        else if ( fh->h_datalen == FH_DATALEN_STRING ) // datalen = -1 => opaque is string
        {
            new_opaque_obj = strdup(block);
            if (new_opaque_obj == NULL)
            {
                _fh_unlock(fh, i);
                return (FH_NO_MEMORY);
            }
        }

        new_h_slot->opaque_obj = new_opaque_obj;
    }

    fh_slot *old_first_h_slot = fh->hash_table[i].h_slot;

    fh->hash_table[i].h_slot = new_h_slot;
    new_h_slot->next = old_first_h_slot;

    _fh_unlock(fh, i);

    _fh_lock_fh(fh);
    (fh->h_elements)++;
    _fh_unlock_fh(fh);

    return (i);
}

// fh_del - remove item from hash and free memory
int fh_del(fh_t *fh, char *key)
{
    int i;
    register fh_slot *h_slot, *prev_h_slot = NULL;
    FH_CHECK(fh);
    FH_KEY_CHECK(key);

    // looking for slot

    i = fh->hash_function(key, fh->h_dim);
    _fh_lock(fh, i);

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
        _fh_unlock(fh, i);
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

    // cleanup only for fixed size or string opaque object
    if ((h_slot->opaque_obj) && (fh->h_datalen != FH_DATALEN_VOIDP))
    {
        free(h_slot->opaque_obj);
    }

    if ( (fh->h_attr & FH_SETATTR_DONTCOPYKEY) == 0 )
    {
        free(h_slot->key);
    }

    free(h_slot);
    h_slot = NULL;

    _fh_unlock(fh, i);

    _fh_lock_fh(fh);
    (fh->h_elements)--;
    _fh_unlock_fh(fh);

    return (i);
}

// serch key and copy out opaque data
// should not be called with datalen = FH_DATALEN_VOIDP (no way to return data)
int fh_search(fh_t *fh, char *key, void *block, int block_size)
{
    int i;
    register fh_slot *h_slot;
    FH_CHECK(fh);
    FH_KEY_CHECK(key);
    if(!block)
        return(FH_BUFFER_NULL);

    if ( fh->h_datalen == FH_DATALEN_VOIDP )
    {
        // do not use this call when datalen is 0
        return (FH_WRONG_DATALEN);
    }


    i = fh->hash_function(key, fh->h_dim);
    _fh_lock(fh, i);

    assert(i<fh->h_dim);

    h_slot = fh->hash_table[i].h_slot;

    while ((h_slot != NULL) && (strcmp(h_slot->key, key)))
    {
        h_slot = h_slot->next;
    }

    if ( h_slot == NULL )
    {
        _fh_unlock(fh, i);
        return (FH_ELEMENT_NOT_FOUND);
    }

    // copy out
    if (h_slot->opaque_obj)
    {
        if ( fh->h_datalen > 0 ) // fixed size opaque_obj
        {
            memcpy(block, h_slot->opaque_obj, fh->h_datalen);
        }
        else if ( fh->h_datalen == FH_DATALEN_STRING ) // copy string
        {
            if(block_size < 0)
            {
                _fh_unlock(fh, i);
                return(FH_DIM_INVALID);
            }

            strncpy(block, h_slot->opaque_obj, block_size);
        }
    }

    _fh_unlock(fh, i);
    return (i);
}

// search the hash and return pointer to the opaque_obj or NULL
void *fh_get(fh_t *fh, char *key, int *error)
{
    if(!fh || fh->h_magic != FH_MAGIC_ID)
    {
        *error = FH_BAD_HANDLE;
        return NULL;
    }

    if(!key || key[0] == '\0')
    {
        *error = FH_INVALID_KEY;
        return NULL;
    }

    int i;
    register fh_slot *h_slot;
    void *opaque = NULL;

    i = fh->hash_function(key, fh->h_dim);
    _fh_lock(fh, i);

    assert(i<fh->h_dim);

    h_slot = fh->hash_table[i].h_slot;

    while ((h_slot != NULL) && (strcmp(h_slot->key, key)))
    {
        h_slot = h_slot->next;
    }

    if ( h_slot == NULL )
    {
        *error = FH_ELEMENT_NOT_FOUND;
        _fh_unlock(fh, i);
        return (NULL);
    }
    opaque = h_slot->opaque_obj;
    _fh_unlock(fh, i);
    *error = FH_OK;
    return (opaque);
}


// search and return locked pointer to opaque
// allows modifying an opaque object entry without del/insert
void *fh_searchlock(fh_t *fh, char *key, int *slot, int *error)
{
    int i;
    register fh_slot *h_slot;

    if(!fh || fh->h_magic != FH_MAGIC_ID)
    {
        *error = FH_BAD_HANDLE;
        return NULL;
    }

    if(!key || key[0] == '\0')
    {
        *error = FH_INVALID_KEY;
        return NULL;
    }

    i = fh->hash_function(key, fh->h_dim);
    _fh_lock(fh, i);

    assert(i<fh->h_dim);

    h_slot = fh->hash_table[i].h_slot;

    while ((h_slot != NULL) && (strcmp(h_slot->key, key)))
    {
        h_slot = h_slot->next;
    }

    if ( h_slot == NULL )
    {
        *error = FH_ELEMENT_NOT_FOUND;
        _fh_unlock(fh, i);
        return (NULL);
    }
    (*slot) = i;
    *error = FH_OK;
    return (h_slot->opaque_obj);
}

// release a lock left from fh_searchlock
int fh_releaselock(fh_t *fh, int slot)
{
    FH_CHECK(fh);
    _fh_unlock(fh, slot);
    return FH_OK;
}


// start scan and find first full entry in hash returning index and pointer to slot
// call with *index containing 0 to start from beginning
int fh_scan_start(fh_t *fh, int start_index, void **slot)
{
    int i;
    FH_CHECK(fh);

    // _fh_lock(fh); TODO which concurrency strategy ??

    for (i = start_index; i < fh->h_dim; i++)
    {
        if (fh->hash_table[i].h_slot != NULL)
        {
            *slot = fh->hash_table[i].h_slot;
            // _fh_unlock(fh);
            return(i);
        }
    }
    // empty hashtable or start_index points to another index in the hashtable and from that point on no data is present.
    *slot = NULL;
    // _fh_unlock(fh);
    return (FH_ELEMENT_NOT_FOUND);
}

// Enum creation. List of elements is allocated to the number of elements contained in hash table.
// Idx is the current element index, initialized to the first element
// Is_valid indicates if enumerator scan reached the end (0) or not (1).
fh_enum_t *fh_enum_create(fh_t *fh, int sort_order, int *error)
{
    if(!fh || fh->h_magic != FH_MAGIC_ID)
    {
        *error = FH_BAD_HANDLE;
        return NULL;
    }

    // Hashtable empty: don't create enumerator, it's useless
    if(fh->h_elements < 1)
    {
        *error = FH_EMPTY_HASHTABLE;
        return NULL;
    }

    fh_enum_t *fhe = malloc(sizeof(fh_enum_t));
    fhe->elem_list = malloc(sizeof(fh_elem_t) * fh->h_elements);
    fhe->idx = 0;
    fhe->is_valid = 1;
    fhe->magic = FHE_MAGIC_ID;

    int enum_index = 0;
    for (int i = 0; i < fh->h_dim; i++)
    {
        _fh_lock(fh, i);
        if (fh->hash_table[i].h_slot != NULL)
        {
            // Get first element in slot
            fh_slot *current = fh->hash_table[i].h_slot;
            fhe->elem_list[enum_index].key = current->key;
            fhe->elem_list[enum_index].opaque_obj = current->opaque_obj;
            enum_index++;

            // Chek if other elements are set in this slot
            while(current->next != NULL)
            {
                current = current->next;
                fhe->elem_list[enum_index].key = current->key;
                fhe->elem_list[enum_index].opaque_obj = current->opaque_obj;
                enum_index++;
            }
        }
        _fh_unlock(fh, i);
    }
    fhe->size = enum_index;

    if(sort_order == FH_ENUM_SORTED_ASC)
    {
        qsort(fhe->elem_list, enum_index, sizeof(fh_elem_t), fh_ascfunc);
    }
    else if(sort_order == FH_ENUM_SORTED_DESC)
    {
        qsort(fhe->elem_list, enum_index, sizeof(fh_elem_t), fh_descfunc);
    }

    return fhe;
}

// Return is_valid value, if 1 enumerator hasn't reached the end of the list
int fh_enum_is_valid(fh_enum_t *fhe)
{
    FHE_CHECK(fhe);

    return fhe->is_valid;
}

// Increment idx, to get next element from list. If idx isn't lower than list size, enumerator became invalid (has reached the end of list)
int fh_enum_move_next(fh_enum_t *fhe)
{
    FHE_CHECK(fhe);

    fhe->idx++;

    if(fhe->idx >= fhe->size)
    {
        fhe->is_valid = 0;
    }

    return FH_OK;
}

// Get current element in the list.
fh_elem_t *fh_enum_get_value(fh_enum_t *fhe, int *error)
{
    if(!fhe || fhe->magic != FHE_MAGIC_ID)
    {
        *error = FH_BAD_HANDLE;
        return NULL;
    }

    if(fhe->is_valid == 1)
    {
        return &fhe->elem_list[fhe->idx];
    }

    return NULL;
}

// Destroy enumerator. Put 0 in magic number (so validity checks on handle will fail), free element list and handle pointer
int fh_enum_destroy(fh_enum_t *fhe)
{
    FHE_CHECK(fhe);

    fhe->magic = 0;
    free(fhe->elem_list);
    free(fhe);

    return FH_OK;
}

// takes index and slot as point of last scan and starting from there returns next entry (copies key and opaque)
// if index and slot are not valid anymore return FH_ELEMENT_NOT_FOUND but continues
// don't use if datalen = 0 ... won't return opaque pointer
int fh_scan_next(fh_t *fh, int *index, void **slot, char *key, void *block, int block_size)
{
    int i, el_not_found = 0;
    register fh_slot *h_slot;
    FH_CHECK(fh);

    // _fh_lock(fh); TODO which concurrency strategy ??
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
                // _fh_unlock(fh);

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
            else if ( fh->h_datalen == FH_DATALEN_STRING ) // copy string
            {
                strncpy(block, h_slot->opaque_obj, block_size);
            }
            // do nothing when datalen = 0
        }

        // return next slot

        if (h_slot->next != NULL)
        {
            // same index
            *slot = h_slot->next;
            // _fh_unlock(fh);
            return (1);
        }


        for (i = (*index) + 1; i < fh->h_dim; i++)
        {
            // finding next full h_slot

            if (fh->hash_table[i].h_slot != NULL)
            {
                *slot = fh->hash_table[i].h_slot;
                (*index) = i;
                // _fh_unlock(fh);
                return (1);
            }
        }
    }
    // _fh_unlock(fh);

    // end of hash
    *slot = NULL;

    if (el_not_found == 1)
    {
        return (FH_SCAN_END);
    }
    else
    {
        return (FH_OK);
    }
}
