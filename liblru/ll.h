/*
 * Copyright (c) 2003, Paul Stephen Borile
 * All rights reserved.
 */

#ifndef LL_H
#define LL_H

#ifdef __cplusplus
extern "C" {
#endif

/** hash magic */
#define LL_MAGIC_ID         0xF0CA

#define LL_OK           1
#define LL_ERROR_LAST_SLOT_INCONSISTENCY -10
#define LL_ERROR_TOP_SLOT_INCONSISTENCY -20
#define LL_ERROR_INDEX_OUT_OF_RANGE -30
#define LL_BAD_LL -40



// ll node struct
typedef struct _ll_slot_t ll_slot_t;
struct _ll_slot_t {
    ll_slot_t *next;
    ll_slot_t *prev;
    void *payload;
    int status;
};

// lru object
struct _ll_t {
    int magic;
    int size;  // maximum number of objects
    ll_slot_t *top, *last, *freelist, *save;
    pthread_mutex_t lock;
};
typedef struct _ll_t ll_t;


// create lru
ll_t *ll_create(int dim, int payload_size);
int ll_destroy(ll_t *ll); // frees all slots in list and freelist
ll_slot_t *ll_remove_last(ll_t *ll, void **payload); // unlinks from last and updates last returning payload and slot
ll_slot_t *ll_slot_new(ll_t *ll, void **payload); // gets a slot from the freelist and returns payload and slot
void ll_slot_free(ll_t *ll, ll_slot_t *slot); // puts slot back in freelist
void ll_slot_move_to_top(ll_t *ll, ll_slot_t *slot); // moves to top
void ll_slot_add_to_top(ll_t *ll, ll_slot_t *slot); // add new slot to top
void ll_print(ll_t *ll, int (*payload_print)(void *)); // print data

// test only: retrieve data from given node. sequential (slow) access.
int ll_get_payload(ll_t *ll, int idx, void **payload);

#ifdef __cplusplus
}
#endif

#endif
