/*
 * Copyright (c) 2003, Paul Stephen Borile
 * All rights reserved.
 */

#ifndef LRU_H
#define LRU_H

#ifdef __cplusplus
extern "C" {
#endif

/** hash magic */
#define LRU_MAGIC_ID			0xC0CCA

#define LRU_OK			1
// errors
#define LRU_BAD_HANDLE	-1 // handle null of not pointing to ft_h
#define LRU_ELEMENT_NOT_FOUND	-2000 // hash element not found
#define LRU_DUPLICATED_ELEMENT	-3000 // duplicated key
#define LRU_NO_MEMORY	-4000 // malloc/calloc fails
#define LRU_WRONG_DATALEN	-6000 // wrong datalen for LRU_search, use LRU_get
#define LRU_DIM_INVALID	-7000 // bad dimension
#define LRU_BAD_ATTR		-9000 // bad attribute to get/setattr

// getattr commands
#define LRU_ATTR_ELEMENT  100 // getattr elements in hash
#define LRU_ATTR_DIM	101 // getattr real dim of hashtable
#define LRU_ATTR_COLLISION	102 // getattr collisions in insert

// lru object
struct _lru_t{
	int	 magic;
	int  size; // maximum number of object kept in hash
	int  attr; // holding attributes
	fh_t *fh;
  ll_t *ll;
};
typedef struct _lru_t lru_t;

// create lru
lru_t	*lru_create(int dim);
int 	lru_add(lru_t *lru, char *key, void *payload);
int 	lru_get(lru_t *lru, char *key, void **payload);
int 	lru_setattr(lru_t *lru, int attr, int value);
int 	lru_getattr(lru_t *lru, int attr, int *value);
int 	lru_destroy(lru_t *lru);

#ifdef __cplusplus
}
#endif

#endif
