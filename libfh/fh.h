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

#define FH_OK           1   // No error
#define FH_BAD_HANDLE   -1  // handle null of not pointing to ft_h
#define FH_INVALID_KEY  -1000 // NULL key
#define FH_ELEMENT_NOT_FOUND    -2000 // hash element not found
#define FH_DUPLICATED_ELEMENT   -3000 // duplicated key
#define FH_NO_MEMORY    -4000 // malloc/calloc fails
#define FH_BUFFER_NULL    -5000 // buffer passed null (fh_search)
#define FH_WRONG_DATALEN    -6000 // wrong datalen for fh_search, use fh_get
#define FH_DIM_INVALID  -7000 // bad dimension
#define FH_BAD_ATTR     -9000 // bad attribute to get/setattr
#define FH_FREE_NOT_REQUESTED   -1500 // free function passed to fh_clean but datalen value is not FH_DATALEN_VOIDP
#define FH_EMPTY_HASHTABLE   -2500 // Call fh_enum_create on an empty fh
#define FH_SCAN_END     -100 // fh_scan_next hash reached end of hash

#define FH_ATTR_ELEMENT  100 // getattr elements in hash
#define FH_ATTR_DIM 101 // getattr real dim of hashtable
#define FH_ATTR_COLLISION   102 // getattr collisions in insert
// attributes to set
#define FH_SETATTR_DONTCOPYKEY  1 // for setattr : do not allocate and copy key

// datalen values
#define FH_DATALEN_STRING   -1
#define FH_DATALEN_VOIDP        0

// Sort order of enumerator
#define FH_ENUM_SORTED_ASC  1
#define FH_ENUM_SORTED_DESC 2

// Tablesize limit under which there will be only one mutex
#define SIZE_LIMIT_SINGLE_MUTEX     64

/*  Local types                                 */

/** single structures used to contain data */
struct _fh_slot {
    char *key;
    struct _fh_slot *next;
    void *opaque_obj;
};
typedef struct _fh_slot fh_slot;

// closed hashing : array of structs with pointer to slots
struct _f_hash {
    fh_slot *h_slot;
};
typedef struct _f_hash f_hash;

// fh object
struct _fh_t {
    int h_magic;
    int h_dim;  // hash size
    int h_datalen;  // opaque_obj size : -1 for strings, 0 for pointers, > 0 for data
    pthread_mutex_t fh_lock;
    int h_elements;  // elements in hash
    int h_collision;  // collisions during insert
    int h_attr;  // holding attributes
    unsigned int (*hash_function)();
    // pthread_mutex_t	h_lock[FH_MAX_CONCURRENT_OPERATIONS];
    // Dynamically allocated mutex pool and its current size
    pthread_mutex_t *h_lock;
    int n_lock;
    f_hash *hash_table;
};
typedef struct _fh_t fh_t;

// create fh : opaque_len = -1 means opaque is string (0 terminted)
fh_t    *fh_create( int dim, int opaque_len, unsigned int (*hash_function)());
int     fh_setattr(fh_t *fh, int attr, int value);
int     fh_getattr(fh_t *fh, int attr, int *value);
int     fh_destroy(fh_t *fh );
// inserts opaque pointed data allocating/copying it inside hastable
int     fh_insert(fh_t *fh, char *key, void *opaque);
// remove entry, free data
int     fh_del(fh_t *fh, char *key );
// search and copy out data
int     fh_search(fh_t *fh, char *key, void *opaque, int opaque_size);
// search and return pointer to internal allocated data
void *fh_get(fh_t *fh, char *key, int *error);

int     fh_scan_start(fh_t *fh, int pos, void **slot);
// scans and copies out data
int     fh_scan_next(fh_t *fh, int *pos, void **slot, char *key, void *opaque, int opaque_size);
// experimental
void *fh_searchlock(fh_t *fh, char *key, int *slot, int *error);
int fh_releaselock(fh_t *fh, int slot);

struct _fh_elem_t {
    char *key;
    void *opaque_obj;
};
typedef struct _fh_elem_t fh_elem_t;

struct _fh_enum_t
{
    int magic;  // Magic number
    int idx;        // index of current element
    fh_elem_t *elem_list;       // list of elements
    int size;       // number of elements
    int is_valid;       // valid flag for enumerator (1 = valid; 0 = not valid)
};
typedef struct _fh_enum_t fh_enum_t;

// New methods for scan completely the hashtable.
fh_enum_t *fh_enum_create(fh_t *fh, int sort_order, int *error);
int fh_enum_is_valid(fh_enum_t *fhe);
int fh_enum_move_next(fh_enum_t *fhe);
fh_elem_t *fh_enum_get_value(fh_enum_t *fhe, int *error);
// fh_elem_t **fh_enum_get_all_values(fh_enum_t *fhe);
int fh_enum_destroy(fh_enum_t *fhe);

typedef int fh_opaque_delete_func (void *);

// Clean hashtable
int fh_clean(fh_t *fh, fh_opaque_delete_func (*del_func));

#ifdef __cplusplus
}
#endif

#endif
