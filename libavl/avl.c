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

#include    "avl.h"

extern avl_node *avl_new_node(const char *key, void *data);
extern avl_node *avl_rotate_right(avl_node *y);
extern avl_node *avl_rotate_left(avl_node *x);
extern avl_node *avl_new_node(const char *key, void *data);
extern int avl_update_height(avl_node *n);
extern int avl_get_balance(avl_node *n);

// avl_create : allocate a new handle or use the passed handle
avl_h *avl_create(avl_h *static_handle)
{
    if (static_handle == NULL)
    {
        static_handle = (avl_h *)malloc(sizeof(avl_h));
        if (static_handle == NULL)
        {
            return NULL;
        }
        static_handle->allocated = 1;
    }
    static_handle->root = NULL;
    return static_handle;
}

// avl_destroy_node : recursively destroy the AVL tree
static void avl_destroy_node(avl_node *node)
{
    if (node == NULL)
    {
        return;
    }
    avl_destroy_node(node->left);
    avl_destroy_node(node->right);
    free(node->key);  // Free the key
    free(node);       // Free the node itself
}

void avl_destroy(avl_h *ah)
{
    if (ah == NULL)
    {
        return;
    }

    avl_destroy_node(ah->root);

    if (ah->allocated == 1)
    {
        free(ah);  // Free the AVL tree handle
    }
}

// avl_insert_node : recursively insert a new node into the AVL tree
static avl_node *avl_insert_node(avl_node *node, const char *key, void *data)
{
    if (node == NULL)
    {
        return avl_new_node(key, data);
    }

    // Perform standard BST insert
    if (strcmp(key, node->key) < 0)
    {
        node->left = avl_insert_node(node->left, key, data);
    }
    else if (strcmp(key, node->key) > 0)
    {
        node->right = avl_insert_node(node->right, key, data);
    }
    else{
        return node;  // No duplicate keys are allowed

    }
    // Update the height of this ancestor node
    node->height = avl_update_height(node);

    // Get balance factor to check whether this node became unbalanced
    int balance = avl_get_balance(node);

    // Perform rotations to balance the tree

    // Left Left Case
    if (balance > 1 && strcmp(key, node->left->key) < 0)
    {
        return avl_rotate_right(node);
    }

    // Right Right Case
    if (balance < -1 && strcmp(key, node->right->key) > 0)
    {
        return avl_rotate_left(node);
    }

    // Left Right Case
    if (balance > 1 && strcmp(key, node->left->key) > 0)
    {
        node->left = avl_rotate_left(node->left);
        return avl_rotate_right(node);
    }

    // Right Left Case
    if (balance < -1 && strcmp(key, node->right->key) < 0)
    {
        node->right = avl_rotate_right(node->right);
        return avl_rotate_left(node);
    }

    // Return the (unchanged) node pointer
    return node;
}

// avl_insert : insert a new node into the AVL tree
int avl_insert(avl_h *ah, const char *key, void *data)
{
    if (ah == NULL)
    {
        return -1;
    }
    ah->root = avl_insert_node(ah->root, key, data);
    return 0;  // Success
}

//  avl_search_node : recursively search for a node in the AVL tree
static void *avl_search_node(avl_node *node, const char *key)
{
    if (node == NULL)
    {
        return NULL;
    }

    if (strcmp(key, node->key) == 0)
    {
        return node->data;
    }
    else if (strcmp(key, node->key) < 0)
    {
        return avl_search_node(node->left, key);
    }
    else{
        return avl_search_node(node->right, key);
    }
}

// avl_search : search for a node in the AVL tree
void *avl_search(avl_h *ah, const char *key)
{
    if (ah == NULL || ah->root == NULL)
    {
        return NULL;
    }
    return avl_search_node(ah->root, key);
}
