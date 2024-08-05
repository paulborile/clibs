/*
   Copyright (c) 2003, Paul Stephen Borile
   All rights reserved.
   License : MIT

 */

// WARNING!!!! Needed to use strdup in this code. Without it, it's not defined
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

#include    "fh.h"

// wyhash used by Go, Zig and more
#include  "wyhash.h"

// version
static char version[] = "1.2.0";

static void wyhash_hash_init(fh_t *fh)
{
    fh->seed = time(NULL);
    make_secret(fh->seed, fh->secret);
}

uint64_t fh_default_hash(void *data, char *key)
{
    fh_t *fh = (fh_t *)data;
    if (key[0] == '\0' )
    {
        // return fixed value on empty string
        return 1;
    }
    uint64_t h = wyhash(key, strlen(key), fh->seed, fh->secret);
    return h;
}

/* *INDENT-OFF* */
#define FH_CHECK(f) if ((!f) || (f->h_magic != FH_MAGIC_ID)) return (FH_BAD_HANDLE)
#define FH_KEY_CHECK(key) if (!key) return (FH_INVALID_KEY)
#define FHE_CHECK(f) if ((!f) || (f->magic != FHE_MAGIC_ID)) return (FH_BAD_HANDLE)

// make mini hash
// we want to reserve some values of tophash for further use
#define MIN_MINIHASH    5 // so we have 0->4 values for future use
#define MAKE_MINIHASH(x,r) {r = x >> 56; if (r < MIN_MINIHASH) r += MIN_MINIHASH;} 
/* *INDENT-ON* */
//
// makes sure the real size of the buckets array is a power of 2 so we can use && for hash truncation
// adds 1.5 size factor to allow for enough space to limit collisions
unsigned int fh_hash_size(unsigned int s)
{
    unsigned int i = 1;
    s = s + s/2; // add 50%
    while (i < s) i <<= 1;
    return i;
}

// Compare functions to sort enumerator
int fh_ascfunc(const void *a, const void *b)
{
    return ( strcmp(((fh_elem_t *)a)->key, ((fh_elem_t *)b)->key) );
}

int fh_descfunc(const void *a, const void *b)
{
    return ( strcmp(((fh_elem_t *)b)->key, ((fh_elem_t *)a)->key) );
}

// standard sharded locks
static void _fh_lock(void *f, int slot)
{
    fh_t *fh = (fh_t *)f;
    pthread_mutex_lock(&(fh->h_lock[slot % fh->n_lock]));
}

static void _fh_unlock(void *f, int slot)
{
    fh_t *fh = (fh_t *)f;
    pthread_mutex_unlock(&(fh->h_lock[slot % fh->n_lock]));
}

// testing rw_locks : read locks to be used in fh_search(), fh_get()

static void _fh_rdlock(void *f, int slot)
{
    fh_t *fh = (fh_t *)f;
    pthread_rwlock_rdlock(&(fh->rw_lock[slot % fh->n_lock]));
}

// write locks : to be used in fh_insert(), fh_delete()
static void _fh_wrlock(void *f, int slot)
{
    fh_t *fh = (fh_t *)f;
    pthread_rwlock_wrlock(&(fh->rw_lock[slot % fh->n_lock]));
}

// only one unlock function for read and write locks
static void _fh_rwunlock(void *f, int slot)
{
    fh_t *fh = (fh_t *)f;
    pthread_rwlock_unlock(&(fh->rw_lock[slot % fh->n_lock]));
}

// create hashtable object and init all data
// datalen = -1 : opaque is a string so it spave for it will be allocated with malloc
// and it will be copied with strcpy.
// datalen > 0 : opaque is a fixed size data structure (struct?); will be allocated with malloc
// and copied with memcpy
// datalen = 0 : opaque is threated as a void pointer, no allocation is done, pointer value is kept in hashtable opa	que data.

fh_t *fh_create(int dim, int datalen, fh_hash_fun hash_function)
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

    f->h_magic = FH_MAGIC_ID;
    f->h_attr = 0;

    if (hash_function != NULL)
    {
        f->hash_function = hash_function;
    }
    else
    {
        wyhash_hash_init(f); // init the hash seed to avoid DOS attacks
        f->hash_function = fh_default_hash; // now default hash is wyhash
    }

    // compute pool size (function of hashtable size), save size in fh object, allocate lock pool, init locks
    if (dim > SIZE_LIMIT_SINGLE_MUTEX)
    {
        // Compute mutex number: it's base 2 logarythm of hashtable dimension
        int num = dim;
        int size = 0;

        while (num > 0)
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

    // default is using standard locks

    f->rlock_f = _fh_lock;
    f->wlock_f = _fh_lock;
    f->unlock_f = _fh_unlock;

    f->h_lock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t) * f->n_lock);
    if (f->h_lock == NULL)
    {
        free(hash);
        free(f);
        return NULL;
    }

    // set to null the other mutex pool
    f->rw_lock = NULL;

    // init hashtable critical region mutexes
    for (int i = 0; i < f->n_lock; i++)
    {
        pthread_mutex_init(&(f->h_lock[i]), NULL);
    }

    pthread_mutex_init(&(f->e_lock), NULL);

    f->hash_table = hash;

    return (f);
}


static inline void _fh_inc_elem(fh_t *fh)
{
    pthread_mutex_lock(&(fh->e_lock));
    fh->h_elements++;
    pthread_mutex_unlock(&(fh->e_lock));
}

static inline void _fh_inc_collision(fh_t *fh)
{
    pthread_mutex_lock(&(fh->e_lock));
    fh->h_collision++;
    pthread_mutex_unlock(&(fh->e_lock));
}

static inline void _fh_dec_elem(fh_t *fh)
{
    pthread_mutex_lock(&(fh->e_lock));
    fh->h_elements--;
    pthread_mutex_unlock(&(fh->e_lock));
}

static inline int _fh_get_elem(fh_t *fh)
{
    pthread_mutex_lock(&(fh->e_lock));
    int ret = fh->h_elements;
    pthread_mutex_unlock(&(fh->e_lock));
    return ret;
}

static inline int _fh_get_collision(fh_t *fh)
{
    pthread_mutex_lock(&(fh->e_lock));
    int ret = fh->h_collision;
    pthread_mutex_unlock(&(fh->e_lock));
    return ret;
}

// set attributes in object
int fh_setattr(fh_t *fh, int attr, int value)
{
    (void) value;
    FH_CHECK(fh);

    switch (attr)
    {
    case FH_SETATTR_DONTCOPYKEY:
        if (_fh_get_elem(fh) > 0)
        {
            return FH_ERROR_OPERATION_NOT_PERMITTED;
        }
        fh->h_attr |= FH_SETATTR_DONTCOPYKEY;
        break;
    case FH_SETATTR_USERWLOCKS:

        // no resetting (value = 0) of USERWLOCKS mode
        if (value == 0)
        {
            return FH_ERROR_OPERATION_NOT_PERMITTED;
        }

        // check if rwlocks already allocated or we are already in RWLOCKS mode
        // we do nothing
        if (fh->rw_lock != NULL || fh->h_attr & FH_SETATTR_USERWLOCKS)
        {
            return FH_ERROR_OPERATION_NOT_PERMITTED;
        }
        // allocate rwlocks pool
        fh->rw_lock = (pthread_rwlock_t *)malloc(sizeof(pthread_rwlock_t) * fh->n_lock);
        if (fh->rw_lock == NULL)
        {
            return FH_NO_MEMORY;
        }
        // init
        for (int i = 0; i < fh->n_lock; i++)
        {
            pthread_rwlock_init(&(fh->rw_lock[i]), NULL);
        }
        // set flag in h_attr
        fh->h_attr |= FH_SETATTR_USERWLOCKS;
        // set lock functions
        fh->rlock_f = _fh_rdlock;
        fh->wlock_f = _fh_wrlock;
        fh->unlock_f = _fh_rwunlock;

        // now cleanup old mutex pool
        for (int i = 0; i < fh->n_lock; i++)
        {
            pthread_mutex_destroy(&(fh->h_lock[i]));
        }
        free (fh->h_lock);
        fh->h_lock = NULL;
        return FH_OK;

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

    switch (attr)
    {
    case FH_ATTR_ELEMENT:
        (*value) = _fh_get_elem(fh);
        break;
    case FH_ATTR_DIM:
        (*value) = fh->h_dim;
        break;
    case FH_ATTR_COLLISION:
        (*value) = _fh_get_collision(fh);
        break;
    default:
        return(FH_BAD_ATTR);
    }
    return (FH_OK);
}

// get string type attributes
int fh_getattr_string(fh_t *fh, int attr, char **value)
{
    FH_CHECK(fh);

    switch (attr)
    {
    case FH_ATTR_VERSION:
        *value = version;
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
    if (del_func != NULL && fh->h_datalen != FH_DATALEN_VOIDP)
    {
        return FH_FREE_NOT_REQUESTED;
    }

    fh_enum_t *fhe = fh_enum_create(fh, 0, &result);

    // If enumerator is NULL something goes wrong. Return error to caller.
    if (fhe == NULL)
    {
        return result;
    }
#ifdef DEBUG
    int e = _fh_get_elem(fh);
    if ( e != fhe->size)
    {
        printf("*** ERROR fh_clean() - fh->elements = %d, fhe->size = %d \n", e, fhe->size);
    }
#endif
    while ( fh_enum_is_valid(fhe) )
    {
        fh_elem_t *element = fh_enum_get_value(fhe, &result);

        fh_del(fh, element->key);

        // free opaque object only if was allocated
        if (fh->h_datalen == FH_DATALEN_VOIDP)
        {
            // If delete object function is passed, use it to delete opaque. Otherwise do nothing
            if (del_func != NULL)
            {
                del_func(element->opaque_obj);
            }
        }

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

    // dealloc hash table
    free(fh->hash_table);

    // free readwrite mutexes if they exist
    if (fh->rw_lock != NULL)
    {
        for (int i = 0; i < fh->n_lock; i++)
        {
            pthread_rwlock_destroy(&fh->rw_lock[i]);
        }
        free(fh->rw_lock);
    }

    // free readwrite mutexes if they exist
    if (fh->h_lock != NULL)
    {
        for (int i = 0; i < fh->n_lock; i++)
        {
            pthread_mutex_destroy(&fh->h_lock[i]);
        }
        // Free mutex pool
        free(fh->h_lock);
    }

    // dealloc struct hash info
    free(fh);

    return (FH_OK);
}

// _fh_allocate_bucket() - helper for allocating fh_bucket and clearing only needed data
static fh_bucket *_fh_allocate_bucket()
{
    fh_bucket *r = malloc(sizeof(fh_bucket));
    if (r == NULL)
        return NULL;
    for (int i=0; i<BUCKET_SIZE; i++)
    {
        r->mini_hashes[i] = 0;
        r->slot[i].key = NULL; // clean keys
        r->slot[i].opaque_obj = NULL;
    }
    r->next = NULL;
    return r;
}

// prototypes
static fh_slot *_fh_return_empty_slot_in_bucket(fh_bucket *b, uint8_t mini_hash);
static int _fh_find_slot_in_bucket(fh_bucket *b, char *key, uint8_t mini_hash, fh_bucket **ret_bucket);

// insert : copies both key and opaque data. If slot already used
// allocates a newone and incremets h_collision
int fh_insert(fh_t *fh, char *key, void *block)
{
    uint64_t hashindex;
    fh_bucket *bucket, *actual_bucket = NULL;
    fh_slot *found_h_slot = NULL;
    void *new_opaque_obj = NULL;
    FH_CHECK(fh);
    FH_KEY_CHECK(key);

    // looking for slot

    uint64_t h = fh->hash_function(fh, key);
    uint8_t mini_hash;
    MAKE_MINIHASH(h, mini_hash); // get the 8 most sign bits
    hashindex = h & (fh->h_dim -1);

    fh->wlock_f(fh, hashindex);

    assert(hashindex<(uint64_t)fh->h_dim);

    bucket = fh->hash_table[hashindex].h_bucket;

    if (bucket != NULL)
    {
        _fh_inc_collision(fh);
    }
    else
    {
        bucket = fh->hash_table[hashindex].h_bucket = _fh_allocate_bucket();
        if ( bucket == NULL )
        {
            fh->unlock_f(fh, hashindex);
            return (FH_NO_MEMORY);
        }
    }

    // check for duplicated element like we do in fh_search/fh_get

    int b_slot = _fh_find_slot_in_bucket(bucket, key, mini_hash, &actual_bucket);

    if ( b_slot != -1 ) // element found
    {
        // we have a duplicate key
        fh->unlock_f(fh, hashindex);
        return (FH_DUPLICATED_ELEMENT);
    }

    //  no duplicate, find a place in bucket for new key/value

    // returns fh_slot *
    found_h_slot = _fh_return_empty_slot_in_bucket(bucket, mini_hash);
    if (found_h_slot == NULL)
    {
        fh->unlock_f(fh, hashindex);
        return (FH_NO_MEMORY);
    }

    // allocate and copy key
    if ( fh->h_attr & FH_SETATTR_DONTCOPYKEY )
    {
        found_h_slot->key = key;
    }
    else
    {
        found_h_slot->key = strdup(key);
        if (found_h_slot->key == NULL)
        {
            fh->unlock_f(fh, hashindex);
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
                fh->unlock_f(fh, hashindex);
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
                fh->unlock_f(fh, hashindex);
                return (FH_NO_MEMORY);
            }
        }

        found_h_slot->opaque_obj = new_opaque_obj;
    }

    fh->unlock_f(fh, hashindex);
    _fh_inc_elem(fh);

    return (hashindex);
}

// like insert but will return pointer (and address) to opaque data just inserted and keep the slot locked
// will also return locked_slot for further use with fh_releaselock()
// be carefull with opaque_obj : should be used only on VOIDP type hashtables (will only be copied in that case)
// you can change the payload of the hash slot with this variable
void *fh_insertlock(fh_t *fh, char *key, void *block, int *locked_slot, int *error, void **opaque_obj_addr)
{
    uint64_t hash_index;
    fh_bucket *bucket, *actual_bucket = NULL;
    fh_slot *found_h_slot = NULL;
    void *new_opaque_obj = NULL;

    if (!fh || fh->h_magic != FH_MAGIC_ID)
    {
        *error = FH_BAD_HANDLE;
        return NULL;
    }

    if (key == NULL)
    {
        *error = FH_INVALID_KEY;
        return NULL;
    }

    // if no address for locked_slot is given we can't continue
    if (locked_slot == NULL)
    {
        *error = FH_INVALID_ARGUMENT;
        return NULL;
    }

    if (opaque_obj_addr != NULL)
    {
        *opaque_obj_addr = NULL;
    }

    // looking for slot

    uint64_t h = fh->hash_function(fh, key);
    uint8_t mini_hash;
    MAKE_MINIHASH(h, mini_hash); // get the 8 most sign bits
    hash_index = h & (fh->h_dim -1);

    fh->wlock_f(fh, hash_index);

    assert(hash_index<(uint64_t)fh->h_dim);

    bucket = fh->hash_table[hash_index].h_bucket;

    if (bucket != NULL)
    {
        _fh_inc_collision(fh);
    }
    else
    {
        bucket = fh->hash_table[hash_index].h_bucket = _fh_allocate_bucket();
        if ( bucket == NULL )
        {
            fh->unlock_f(fh, hash_index);
            *error = FH_NO_MEMORY;
            return NULL;
        }
    }

    // check for duplicated element like we do in fh_search/fh_get

    int b_slot = _fh_find_slot_in_bucket(bucket, key, mini_hash, &actual_bucket);

    if ( b_slot != -1 ) // element found
    {
        // we have a duplicate key
        fh->unlock_f(fh, hash_index);
        *error = FH_DUPLICATED_ELEMENT;
        return NULL;
    }

    //  find a place in bucket for new key/value

    // returns fh_slot *
    found_h_slot = _fh_return_empty_slot_in_bucket(bucket, mini_hash);
    if (found_h_slot == NULL)
    {
        fh->unlock_f(fh, hash_index);
        *error = FH_NO_MEMORY;
        return NULL;
    }

    // allocate and copy key
    if ( fh->h_attr & FH_SETATTR_DONTCOPYKEY )
    {
        found_h_slot->key = key;
    }
    else
    {
        found_h_slot->key = strdup(key);
        if (found_h_slot->key == NULL)
        {
            fh->unlock_f(fh, hash_index);
            *error = FH_NO_MEMORY;
            return NULL;
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
                fh->unlock_f(fh, hash_index);
                *error = FH_NO_MEMORY;
                return NULL;
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
                fh->unlock_f(fh, hash_index);
                *error = FH_NO_MEMORY;
                return NULL;
            }
        }

        found_h_slot->opaque_obj = new_opaque_obj;
    }

    _fh_inc_elem(fh); // TBV if using it inside main CR is ok

    *error = FH_OK;
    *locked_slot = hash_index;
    if ((opaque_obj_addr != NULL) && (fh->h_datalen == FH_DATALEN_VOIDP))
    {
        *opaque_obj_addr = &(found_h_slot->opaque_obj);
    }
    return found_h_slot->opaque_obj;
}


// _fh_return_empty_slot_in_bucket() - looks for an empty space in bucket chain
// if needed allocates it, mini_hash is set on new empty entry
static fh_slot *_fh_return_empty_slot_in_bucket(fh_bucket *b, uint8_t mini_hash)
{
    // printf("_fh_return_empty_slot_in_bucket(b = %p, mini_hash = %x\n", b, mini_hash);
    while (b != NULL)
    {
        for (int i=0; i<BUCKET_SIZE; i++)
        {
            // look for free entry
            if (b->mini_hashes[i] == 0)
            {
                //empty slot : fill mini_hash
                b->mini_hashes[i] = mini_hash;
                return &(b->slot[i]);
            }
        }
        // reached means no empty or duplicates
        if ( b->next == NULL )
        {
            // we need an empty slot so we must add bucket
            b->next = _fh_allocate_bucket();
            if (b->next == NULL)
                return NULL;                  // no memory
        }
        b = b->next;
    }
    return NULL;
}

static int _fh_del(fh_t *fh, char *key, int bucket_number, uint8_t mini_hash);

// fh_del - remove item from hash and free memory
int fh_del(fh_t *fh, char *key)
{
    int hash_index;
    FH_CHECK(fh);
    FH_KEY_CHECK(key);

    // find bucket
    uint64_t h = fh->hash_function(fh, key);
    uint8_t mini_hash;
    MAKE_MINIHASH(h, mini_hash); // get the 8 most sign bits
    hash_index = h & (fh->h_dim -1);

    fh->wlock_f(fh, hash_index);
    int rc = _fh_del(fh, key, hash_index, mini_hash);
    fh->unlock_f(fh, hash_index);

    if ( rc >= 0 )
    {
        // del returned the hashslot in which the del occurred
        _fh_dec_elem(fh);
    }

    return (rc);
}

// fh_dellocked - remove item from locked hash slot
int fh_dellocked(fh_t *fh, char *key, int locked_bucket)
{
    FH_CHECK(fh);
    FH_KEY_CHECK(key);

    uint64_t h = fh->hash_function(fh, key);
    uint8_t mini_hash;
    MAKE_MINIHASH(h, mini_hash); // get the 8 most sign bits

    int rc = _fh_del(fh, key, locked_bucket, mini_hash);

    if ( rc >= 0 )
    {
        // del returned the hashslot in which the del occurred
        _fh_dec_elem(fh);
    }

    return (rc);
}

// deallocate fh_bucket, start cannot be NULL. Return new address for start (either old one, new or null)
fh_bucket *_deallocate_bucket_if_empty(fh_bucket *candidate, fh_bucket *start)
{
#ifdef DEBUG
    if (start == NULL)
    {
        printf("_deallocate_bucket_if_empty(candidate = %p, start = %p\n", candidate, start);
    }
#endif

    for (int i = 0; i<BUCKET_SIZE; i++)
    {
        if (candidate->mini_hashes[i] != 0 )
        {
            // still data in
            return start;
        }
    }

    // if reached bucket is empty, detach it before freeing it
    fh_bucket *finger = start;
    if ( finger == candidate )
    {
        finger = candidate->next;
        free(candidate);
        return finger; // start changed
    }
    while (finger)
    {
        if ( finger->next == candidate )
        {
            finger->next = candidate->next;
            free(candidate);
            return start;
        }
        finger = finger->next;
    }
    // finger NULL but candidate bucket was not found ..... disaster
    assert(0);
    return start;
}


// internal fh_del which dels with no locks starting on a bucket.
// to be used by both fh_del() and fh_dellock()
static int _fh_del(fh_t *fh, char *key, int hash_index, uint8_t mini_hash)
{
    fh_bucket *bucket, *actual_bucket;

    assert(hash_index<fh->h_dim);

    bucket = fh->hash_table[hash_index].h_bucket;

    // scan until end or element found

    int b_slot = _fh_find_slot_in_bucket(bucket, key, mini_hash, &actual_bucket);

    if ( b_slot == -1 )
    {
        return (FH_ELEMENT_NOT_FOUND);
    }

    // remove the slot
    // clean minihash

    actual_bucket->mini_hashes[b_slot] = 0;

    // cleanup only for fixed size or string opaque object
    if ((actual_bucket->slot[b_slot].opaque_obj) && (fh->h_datalen != FH_DATALEN_VOIDP))
    {
        free(actual_bucket->slot[b_slot].opaque_obj);
    }
    actual_bucket->slot[b_slot].opaque_obj = NULL;

    if ( (fh->h_attr & FH_SETATTR_DONTCOPYKEY) == 0 )
    {
        free(actual_bucket->slot[b_slot].key);
    }
    actual_bucket->slot[b_slot].key = NULL;

    // check if we can deallocate the bucket
    fh->hash_table[hash_index].h_bucket = _deallocate_bucket_if_empty(actual_bucket, bucket);
    return(hash_index);
}

// _fh_find_slot_in_bucket() - finds a slot with give key, returns bucket and which entry (int)
static int _fh_find_slot_in_bucket(fh_bucket *b, char *key, uint8_t mini_hash, fh_bucket **ret_bucket)
{
    // printf("_fh_find_slot_in_bucket(b = %p, key = %s, mini_hash = %x\n", b, key, mini_hash);

    while (b != NULL)
    {
        for (int i=0; i<BUCKET_SIZE; i++)
        {
            // chek for a match
            if (b->mini_hashes[i] == mini_hash)
            {
                // candidate match
                if (strcmp(b->slot[i].key, key) == 0)
                {
                    // return it
                    *ret_bucket = b;
                    return i;
                }
            }
        }
        // reached means not found in first bucket, check if chain present
        if ( b->next == NULL )
        {
            return -1; // no further chain
        }
        b = b->next;
    }
    // b was null from the beginning case
    return -1;
}

// serch key and copy out opaque data
// should not be called with datalen = FH_DATALEN_VOIDP (no way to return data)
int fh_search(fh_t *fh, char *key, void *block, int block_size)
{
    uint64_t i;
    fh_bucket *bucket, *actual_bucket;

    FH_CHECK(fh);
    FH_KEY_CHECK(key);
    if (!block)
        return(FH_INVALID_ARGUMENT);

    if ( fh->h_datalen == FH_DATALEN_VOIDP )
    {
        // do not use this call when datalen is 0
        return (FH_WRONG_DATALEN);
    }

    // find bucket
    uint64_t h = fh->hash_function(fh, key);
    uint8_t mini_hash;
    MAKE_MINIHASH(h, mini_hash); // get the 8 most sign bits
    i = h & (fh->h_dim -1);

    fh->rlock_f(fh, i);

    assert(i<(uint64_t)fh->h_dim);

    bucket = fh->hash_table[i].h_bucket;

    int b_slot = _fh_find_slot_in_bucket(bucket, key, mini_hash, &actual_bucket);

    if ( b_slot == -1 )
    {
        fh->unlock_f(fh, i);
        return (FH_ELEMENT_NOT_FOUND);
    }

    // copy out
    if (actual_bucket->slot[b_slot].opaque_obj)
    {
        if ( fh->h_datalen > 0 ) // fixed size opaque_obj
        {
            memcpy(block, actual_bucket->slot[b_slot].opaque_obj, fh->h_datalen);
        }
        else if ( fh->h_datalen == FH_DATALEN_STRING ) // copy string
        {
            if (block_size < 0)
            {
                fh->unlock_f(fh, i);
                return(FH_DIM_INVALID);
            }

            strncpy(block, actual_bucket->slot[b_slot].opaque_obj, block_size);
        }
    }

    fh->unlock_f(fh, i);
    return (i);
}

// search the hash and return pointer to the opaque_obj or NULL
void *fh_get(fh_t *fh, char *key, int *error)
{
    fh_bucket *bucket, *actual_bucket;

    if ((fh == NULL) || (fh->h_magic != FH_MAGIC_ID))
    {
        *error = FH_BAD_HANDLE;
        return NULL;
    }

    if (key == NULL)
    {
        *error = FH_INVALID_KEY;
        return NULL;
    }

    uint64_t i;
    void *opaque = NULL;

    // find bucket
    uint64_t h = fh->hash_function(fh, key);
    uint8_t mini_hash;
    MAKE_MINIHASH(h, mini_hash); // get the 8 most sign bits
    i = h & (fh->h_dim -1);

    fh->rlock_f(fh, i);

    assert(i<(uint64_t)fh->h_dim);

    bucket = fh->hash_table[i].h_bucket;

    int b_slot = _fh_find_slot_in_bucket(bucket, key, mini_hash, &actual_bucket);

    if ( b_slot == -1 )
    {
        fh->unlock_f(fh, i);
        *error = FH_ELEMENT_NOT_FOUND;
        return NULL;
    }

    opaque = actual_bucket->slot[b_slot].opaque_obj;
    fh->unlock_f(fh, i);
    *error = FH_OK;
    return (opaque);
}


// search and return locked pointer to opaque
// allows modifying an opaque object entry without del/insert
void *fh_searchlock(fh_t *fh, char *key, int *slot, int *error)
{
    uint64_t i;
    fh_bucket *bucket, *actual_bucket;

    if (!fh || fh->h_magic != FH_MAGIC_ID)
    {
        *error = FH_BAD_HANDLE;
        return NULL;
    }

    if (!key)
    {
        *error = FH_INVALID_KEY;
        return NULL;
    }

    // find bucket
    uint64_t h = fh->hash_function(fh, key);
    uint8_t mini_hash;
    MAKE_MINIHASH(h, mini_hash); // get the 8 most sign bits
    i = h & (fh->h_dim -1);

    fh->wlock_f(fh, i);

    assert(i<(uint64_t)fh->h_dim);

    bucket = fh->hash_table[i].h_bucket;

    int b_slot = _fh_find_slot_in_bucket(bucket, key, mini_hash, &actual_bucket);

    if ( b_slot == -1 )
    {
        fh->unlock_f(fh, i);
        *error = FH_ELEMENT_NOT_FOUND;
        return NULL;
    }
    (*slot) = i;
    *error = FH_OK;
    return (actual_bucket->slot[b_slot].opaque_obj);
}

// release a lock left from fh_searchlock
// int fh_releaselock(fh_t *fh, int slot)
// {
//     FH_CHECK(fh);
//     _fh_unlock(fh, slot);
//     return FH_OK;
// }

// release a lock left from fh_searchlock
int fh_release_searchlock(fh_t *fh, int slot)
{
    FH_CHECK(fh);
    fh->unlock_f(fh, slot);
    return FH_OK;
}

// release a lock left from fh_insertlock
int fh_release_insertlock(fh_t *fh, int slot)
{
    FH_CHECK(fh);
    fh->unlock_f(fh, slot);
    return FH_OK;
}


// Enum creation. List of elements is allocated to the number of elements contained in hash table.
// Idx is the current element index, initialized to the first element
// Is_valid indicates if enumerator scan reached the end (0) or not (1).
fh_enum_t *fh_enum_create(fh_t *fh, int sort_order, int *error)
{
    if (!fh || fh->h_magic != FH_MAGIC_ID)
    {
        *error = FH_BAD_HANDLE;
        return NULL;
    }

    int elements_in_hash = _fh_get_elem(fh);
    // Hashtable empty: don't create enumerator, it's useless
    if (elements_in_hash < 1)
    {
        *error = FH_EMPTY_HASHTABLE;
        return NULL;
    }
    fh_enum_t *fhe = malloc(sizeof(fh_enum_t));
    // if (fhe == NULL)
    // {
    //     *error = FH_NO_MEMORY;
    //     return NULL;
    // }
    fhe->elem_list = calloc(elements_in_hash, sizeof(fh_elem_t));
    // if (fhe->elem_list == NULL)
    // {
    //     free(fhe);
    //     *error = FH_NO_MEMORY;
    //     return NULL;
    // }
    fhe->idx = 0;
    fhe->is_valid = 1;
    fhe->magic = FHE_MAGIC_ID;

    int enum_index = 0;
    for (int i = 0; (i < fh->h_dim && enum_index < elements_in_hash); i++)
    {
        fh->wlock_f(fh, i);

        if (fh->hash_table[i].h_bucket != NULL)
        {
            // Get first element in slot
            fh_bucket *current = fh->hash_table[i].h_bucket;

            while (current != NULL)
            {
                // loop on bucket slots
                for (int b=0; b<BUCKET_SIZE; b++)
                {
                    if (current->mini_hashes[b] != 0)
                    {
                        // check at every element for overrun
                        if ( enum_index >= elements_in_hash)
                        {
#ifdef DEBUG
                            printf("*** ERROR fh_enum_create() - enum_index = %d, elements_in_hash = %d \n", enum_index, elements_in_hash);
#endif
                            break;
                        }
                        fhe->elem_list[enum_index].key = current->slot[b].key;
                        fhe->elem_list[enum_index].opaque_obj = current->slot[b].opaque_obj;
                        enum_index++;
                    }
                }
                current = current->next;
            }
        }
        fh->unlock_f(fh, i);
    }
    fhe->size = enum_index;

#ifdef DEBUG
    int e = _fh_get_elem(fh);
    if (e != elements_in_hash) // hash size changed in size during enumeration
    {
        printf("*** ERROR fh_enum_create() - size chnaged, fh->elements = %d, elements_in_hash = %d \n", e, elements_in_hash);
    }
#endif

    if (sort_order == FH_ENUM_SORTED_ASC)
    {
        qsort(fhe->elem_list, enum_index, sizeof(fh_elem_t), fh_ascfunc);
    }
    else if (sort_order == FH_ENUM_SORTED_DESC)
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

    if (fhe->idx >= fhe->size)
    {
        fhe->is_valid = 0;
    }

    return FH_OK;
}

// Get current element in the list.
fh_elem_t *fh_enum_get_value(fh_enum_t *fhe, int *error)
{
    if (!fhe || fhe->magic != FHE_MAGIC_ID)
    {
        *error = FH_BAD_HANDLE;
        return NULL;
    }

    if (fhe->is_valid == 1)
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
