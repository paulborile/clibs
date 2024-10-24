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

#ifndef FH_H
#define FH_H

#ifdef __cplusplus
extern "C" {
#endif

/** hash magic */
#define FH_MAGIC_ID         0xCACCA
#define FHE_MAGIC_ID        0xBACCA

#define FH_OK               1   // No error
#define FH_BAD_HANDLE       -1  // handle null of not pointing to ft_h
#define FH_INVALID_KEY      -1000 // NULL key
#define FH_ELEMENT_NOT_FOUND    -2000 // hash element not found
#define FH_DUPLICATED_ELEMENT   -3000 // duplicated key
#define FH_NO_MEMORY        -4000 // malloc/calloc fails
#define FH_INVALID_ARGUMENT      -5000 // buffer passed null (fh_search)
#define FH_WRONG_DATALEN    -6000 // wrong datalen for fh_search, use fh_get
#define FH_DIM_INVALID      -7000 // bad dimension
#define FH_BAD_ATTR         -9000 // bad attribute to get/setattr
#define FH_FREE_NOT_REQUESTED   -1500 // free function passed to fh_clean but datalen value is not FH_DATALEN_VOIDP
#define FH_EMPTY_HASHTABLE  -2500 // Call fh_enum_create on an empty fh
#define FH_ERROR_OPERATION_NOT_PERMITTED    -3500 // invalid operation
#define FH_SCAN_END         -100 // fh_scan_next hash reached end of hash

#define FH_ATTR_ELEMENT     100 // getattr elements in hash
#define FH_ATTR_DIM         101 // getattr real dim of hashtable
#define FH_ATTR_COLLISION   102 // getattr collisions in insert
#define FH_ATTR_VERSION     103 // getattr return library version

// attributes to set in h_attr bitwise
#define FH_SETATTR_DONTCOPYKEY  1 // for setattr : do not allocate and copy key
#define FH_SETATTR_USERWLOCKS   2 // for setattr : use read/write locks

// datalen values
#define FH_DATALEN_STRING   -1
#define FH_DATALEN_VOIDP    0

// Sort order of enumerator
#define FH_ENUM_SORTED_ASC  1
#define FH_ENUM_SORTED_DESC 2

// Tablesize limit under which there will be only one mutex
#define SIZE_LIMIT_SINGLE_MUTEX     64

// bucket/slot size in number of keys
#define BUCKET_SIZE 8
/*  Local types                                 */

/** single structures used to contain data */
struct _fh_slot {
    char *key;
    void *opaque_obj;
};
typedef struct _fh_slot fh_slot;

// bucket contains slots for key/value
struct _fh_bucket {
    uint8_t mini_hashes[BUCKET_SIZE]; // for faster comparison of keys
    fh_slot slot[BUCKET_SIZE];
    struct _fh_bucket *next;
};
typedef struct _fh_bucket fh_bucket;

// closed hashing : array of structs with pointer to slots
struct _f_hash {
    fh_bucket *h_bucket;
};
typedef struct _f_hash f_hash;
typedef uint64_t (*fh_hash_fun)(void *data, char *key);
// for lock functions
typedef void (*fh_lock_func)(void *fh, int slot);

// fh object
struct _fh_t {
    int h_magic;
    int h_dim;  // hash size
    int h_datalen;  // opaque_obj size : -1 for strings, 0 for pointers, > 0 for data
    int h_elements;  // elements in hash
    pthread_mutex_t e_lock; // elements operations lock
    int h_collision;  // collisions during insert
    int h_attr;  // holding attributes
    fh_hash_fun hash_function;
    pthread_mutex_t *h_lock; // standard locks, default
    pthread_rwlock_t *rw_lock; // rwlocks
    fh_lock_func rlock_f; // lock function : these are set at fh_create/modified by setattr
    fh_lock_func wlock_f; // lock function : these are set at fh_create/modified by setattr
    fh_lock_func unlock_f; // unlock function
    int n_lock;
    f_hash *hash_table;
    uint64_t seed; // wyhash seed
    uint64_t secret[4]; // wyhash secret
};
typedef struct _fh_t fh_t;


// create fh : opaque_len = -1 means opaque is string (0 terminted)
fh_t *fh_create( int dim, int opaque_len, fh_hash_fun custom_hash);
int fh_setattr(fh_t *fh, int attr, int value);
int fh_getattr(fh_t *fh, int attr, int *value);
int fh_getattr_string(fh_t *fh, int attr, char **value);
int fh_destroy(fh_t *fh );
// inserts opaque pointed data allocating/copying it inside hastable
int fh_insert(fh_t *fh, char *key, void *opaque);
// remove entry, free data
int fh_del(fh_t *fh, char *key );
// search and copy out data
int fh_search(fh_t *fh, char *key, void *opaque, int opaque_size);
// search and return pointer to internal allocated data
void *fh_get(fh_t *fh, char *key, int *error);

// like fh_get but returns pointer to opaque on a locked slot (return locked slot as well for later fh_releaselock)
void *fh_searchlock(fh_t *fh, char *key, int *locked_slot, int *error);
// like fh_insert but return pointer to opaque data inserted locked : allows to modify opaque data while inserting
void *fh_insertlock(fh_t *fh, char *key, void *opaque, int *locked_slot, int *error, void **opaque_obj);
// fh_dellocked - remove item from a locked hash slot (returned by fh_searchlock)
int fh_dellocked(fh_t *fh, char *key, int locked_slot);
int fh_releaselock(fh_t *fh, int locked_slot);

uint64_t fh_default_hash(void *data, char *key);// compute the hash size given initial dimension
unsigned int fh_hash_size(unsigned int s);

// elements in enumeration
struct _fh_elem_t {
    char *key;
    void *opaque_obj;
};
typedef struct _fh_elem_t fh_elem_t;
// enumeration handle
struct _fh_enum_t
{
    int magic;  // Magic number
    int idx;        // index of current element
    fh_elem_t *elem_list;       // list of elements
    int size;       // number of elements
    int is_valid;       // valid flag for enumerator (1 = valid; 0 = not valid)
};
typedef struct _fh_enum_t fh_enum_t;

// New methods to completely scan the hashtable.
// fh_enum_create takes a snapshot of the hashtable in that moment
// if hashtable size varies during enumeration, it might not find all elements
fh_enum_t *fh_enum_create(fh_t *fh, int sort_order, int *error);
int fh_enum_is_valid(fh_enum_t *fhe);
int fh_enum_move_next(fh_enum_t *fhe);
fh_elem_t *fh_enum_get_value(fh_enum_t *fhe, int *error);
// fh_elem_t **fh_enum_get_all_values(fh_enum_t *fhe);
int fh_enum_destroy(fh_enum_t *fhe);

// typedef defining generic free function parameter in clean method
typedef void fh_opaque_delete_func (void *);

// Clean hashtable
int fh_clean(fh_t *fh, fh_opaque_delete_func (*del_func));

// some hash functions provided
// jsw_hash behaves good with strings
uint64_t jsw_hash(void *, char *);
// low collisions
uint64_t jen_hash(void *, char *);

/* *INDENT-OFF* */
#ifdef __cplusplus
}
#endif
/* *INDENT-ON* */

#endif
