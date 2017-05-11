/*
 * Copyright (c) 2003, Paul Stephen Borile
 * All rights reserved.
 */

#ifndef FH_H
#define FH_H

#ifdef __cplusplus
extern "C" {
#endif

/** hash magic */
#define FH_MAGIC_ID			0xCACCA
/** magic used during destroy */
//#define FH_MAGIC_ID_DESTROY  0xFF00

#define FH_BAD_HANDLE	-1 // handle null of not pointing to ft_h
#define FH_ELEMENT_NOT_FOUND	-2000 // hash element not found
#define FH_DUPLICATED_ELEMENT	-3000 // duplicated key
#define FH_NO_MEMORY	-4000 // malloc/calloc fails
#define FH_WRONG_DATALEN	-6000 // wrong datalen for fh_search, use fh_get
#define FH_DIM_INVALID	-7000 // bad dimension
#define FH_BAD_ATTR		-9000 // bad attribute to get/setattr
#define FH_SCAN_END		-100 // fh_scan_next hash reached end of hash

#define FH_ATTR_ELEMENT  100 // getattr elements in hash
#define FH_ATTR_DIM	101 // getattr real dim of hashtable
#define FH_ATTR_COLLISION	102 // getattr collisions in insert
// attributes to set
#define FH_SETATTR_DONTCOPYKEY	1 // for setattr : do not allocate and copy key

// datalen values
#define FH_DATALEN_STRING	-1
#define FH_DATALEN_VOIDP		0

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

/* header for hasdump - not used */
struct _dump_info {
	// key len
	int key_len;
};
typedef struct _dump_info dump_info;

// fh object
struct _fh_t{
	int	 h_magic;
	int  h_dim; // hash size
	int	 h_datalen; // opaque_obj size : -1 for strings, 0 for pointers, > 0 for data
	int  h_elements; // elements in hash
	int	 h_collision; // collisions during insert
	int  h_attr; // holding attributes
	unsigned int  (*hash_function)();
	pthread_mutex_t	h_lock;
	f_hash *hash_table;
};
typedef struct _fh_t fh_t;

// create fh : opaque_len = -1 means opaque is string (0 terminted)
fh_t	*fh_create( int dim, int opaque_len, unsigned int (*hash_function)());
int 	fh_setattr(fh_t *fh, int attr, int value);
int 	fh_getattr(fh_t *fh, int attr, int *value);
int 	fh_destroy(fh_t *fh );
// inserts opaque pointed data allocating/copying it inside hastable
int 	fh_insert(fh_t *fh, char *key, void *opaque);
// remove entry, free data
int 	fh_del(fh_t *fh, char *key );
// search and copy out data
int 	fh_search(fh_t *fh, char *key, void *opaque, int opaque_size);
// search and return pointer to internal allocated data
void *fh_get(fh_t *fh, char *key, int *error);

int 	fh_scan_start(fh_t *fh, int pos, void **slot);
// scans and copies out data
int 	fh_scan_next(fh_t *fh, int *pos, void **slot, char *key, void *opaque, int opaque_size);
// experimental
void *fh_searchlock(fh_t *fh, char *key, int *slot);
void	fh_releaselock(fh_t *fh, int slot);

/*
int 	fh_dump(fh_t *fh, char *file);
int 	fh_load(fh_t *fh, char *file );
*/

#ifdef __cplusplus
}
#endif

#endif
