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


// Helper function to get the node with the minimum key (used to find successor)
static avl_node *avl_min_value_node(avl_node *node)
{
    avl_node *current = node;
    while (current->left != NULL) {
        current = current->left;
    }
    return current;
}

// Recursive function to delete a node and rebalance the tree
static avl_node *avl_delete_node(avl_node *root, const char *key, void **data)
{
    // Base case: if the tree is empty
    if (root == NULL)
    {
        return root;
    }

    // Perform standard BST delete
    if (strcmp(key, root->key) < 0)
    {
        root->left = avl_delete_node(root->left, key, data);
    }
    else if (strcmp(key, root->key) > 0)
    {
        root->right = avl_delete_node(root->right, key, data);
    }
    else {
        // Node found, store the data before deletion
        *data = root->data;

        // Node with only one child or no child
        if (root->left == NULL || root->right == NULL)
        {
            avl_node *temp = root->left ? root->left : root->right;

            // No child case
            if (temp == NULL)
            {
                temp = root;
                root = NULL;
            }
            else{  // One child case
                *root = *temp; // Copy the contents of the non-empty child

            }
            free(temp->key); // Free the key
            free(temp);      // Free the node
        }
        else {
            // Node with two children: Get the inorder successor (smallest in the right subtree)
            avl_node *temp = avl_min_value_node(root->right);

            // Copy the successor's data to this node
            free(root->key);
            root->key = strdup(temp->key);
            root->data = temp->data;

            // Delete the inorder successor
            root->right = avl_delete_node(root->right, temp->key, &temp->data);
        }
    }

    // If the tree had only one node, return
    if (root == NULL)
    {
        return root;
    }

    // Update the height of the current node
    root->height = avl_update_height(root);

    // Rebalance the tree
    int balance = avl_get_balance(root);

    // Left Left Case
    if (balance > 1 && avl_get_balance(root->left) >= 0)
    {
        return avl_rotate_right(root);
    }

    // Left Right Case
    if (balance > 1 && avl_get_balance(root->left) < 0)
    {
        root->left = avl_rotate_left(root->left);
        return avl_rotate_right(root);
    }

    // Right Right Case
    if (balance < -1 && avl_get_balance(root->right) <= 0)
    {
        return avl_rotate_left(root);
    }

    // Right Left Case
    if (balance < -1 && avl_get_balance(root->right) > 0)
    {
        root->right = avl_rotate_right(root->right);
        return avl_rotate_left(root);
    }

    return root;
}


void *avl_del(avl_h *ah, const char *key)
{
    if (ah == NULL || ah->root == NULL)
    {
        return NULL;
    }

    void *deleted_data = NULL;
    ah->root = avl_delete_node(ah->root, key, &deleted_data);

    return deleted_data;
}
