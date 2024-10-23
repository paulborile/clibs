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

// Get height of the node
int avl_get_height(avl_node *n)
{
    if (n == NULL)
    {
        return 0;
    }
    return n->height;
}

// Update the height of the node
int avl_update_height(avl_node *n)
{
    if (n == NULL)
    {
        return 0;
    }
    int left_height = avl_get_height(n->left);
    int right_height = avl_get_height(n->right);
    return 1 + (left_height > right_height ? left_height : right_height);
}

// Perform a right rotation
avl_node *avl_rotate_right(avl_node *y)
{
    avl_node *x = y->left;
    avl_node *T2 = x->right;

    // Perform rotation
    x->right = y;
    y->left = T2;

    // Update heights
    y->height = avl_update_height(y);
    x->height = avl_update_height(x);

    // Return new root
    return x;
}

// Perform a left rotation
avl_node *avl_rotate_left(avl_node *x)
{
    avl_node *y = x->right;
    avl_node *T2 = y->left;

    // Perform rotation
    y->left = x;
    x->right = T2;

    // Update heights
    x->height = avl_update_height(x);
    y->height = avl_update_height(y);

    // Return new root
    return y;
}

// Get balance factor of node n
int avl_get_balance(avl_node *n)
{
    if (n == NULL)
    {
        return 0;
    }
    return avl_get_height(n->left) - avl_get_height(n->right);
}

// Create a new node
avl_node *avl_new_node(const char *key, void *data)
{
    avl_node *node = (avl_node *)malloc(sizeof(avl_node));
    node->key = strdup(key);  // Copy the key
    node->data = data;
    node->left = node->right = NULL;
    node->height = 1;  // New node is initially added at height 1
    return node;
}
