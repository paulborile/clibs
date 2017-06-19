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
#define LL_MAGIC_ID			0xF0CA

// lru object
struct _ll_t {
	int	 ll_magic;
	int  ll_size; // maximum number of objects
	void *ll_first, *ll_last;
};
typedef struct _ll_t ll_t;

struct _ll_slot_t {
	struct ll_slot_t *next;
	struct ll_slot_t *prev;
};
typedef struct _ll_slot_t ll_slot_t;

// create lru
ll_t	*ll_create(int dim);
int ll_destroy(ll_t *ll); // frees all slots in list and freelist
void *ll_remove_last(ll_t *ll, char **key, void **payload); // unlinks from last and updates last returning payload and key
void *ll_slot_new(ll_t *ll); // gets a slot from the freelist
void ll_slot_set_payload(ll_t *ll, void *slot, void *payload); // sets payload in a slot
void ll_slot_set_key(ll_t *ll, void *slot, char *key); // sets payload in a slot
int ll_slot_free(ll_t *ll, void *slot); // puts slot back in freelist
void ll_slot_move_to_top(ll_t *ll, void *slot); // moves to top

#ifdef __cplusplus
}
#endif

#endif
