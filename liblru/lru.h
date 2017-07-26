/*
 * Copyright (c) 2003, Paul Stephen Borile
 * All rights reserved.
 */

#ifndef LRU_H
#define LRU_H

#ifdef __cplusplus
extern "C" {
#endif

#include "fh.h"

/** hash magic */
#define LRU_MAGIC_ID      0xC0CCA

#define LRU_OK      1
// errors
#define LRU_BAD_HANDLE  -1 // handle null of not pointing to ft_h
#define LRU_ELEMENT_NOT_FOUND  -2000 // hash element not found
#define LRU_DUPLICATED_ELEMENT  -3000 // duplicated key
#define LRU_NO_MEMORY  -4000 // malloc/calloc fails
#define LRU_WRONG_DATALEN  -6000 // wrong datalen for LRU_search, use LRU_get
#define LRU_DIM_INVALID  -7000 // bad dimension
#define LRU_BAD_ATTR    -9000 // bad attribute to get/setattr

// lru object
struct _lru_t{
  int   magic;
  int  size; // maximum number of object kept in hash
  int  attr; // holding attributes
  fh_t *fh;
  void *ll;
};
typedef struct _lru_t lru_t;

// create lru
lru_t  *lru_create(int dim);
int   lru_add(lru_t *lru, char *key, void *payload);
int   lru_check(lru_t *lru, char *key, void **payload);
int   lru_destroy(lru_t *lru);
int   lru_print(lru_t *lru);
int   lru_clear(lru_t *lru);

// test only: retrieve data from ll (the LRU list). sequential (slow) access.
int   lru_get_ll_data(lru_t *lru, int idx, char** key, void** payload, void** ll_slot);
// test only: get index of given key in ll. sequential (slow) access
// returns -1 if key not found
int   lru_get_ll_key_position(lru_t *lru, const char* key);

#ifdef __cplusplus
}
#endif

#endif
