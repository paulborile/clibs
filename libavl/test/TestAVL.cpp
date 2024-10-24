#include <gtest/gtest.h>

#include "avl.h"

#include <pthread.h>
#include <string.h>

#include <thread>
#include <chrono>
using namespace std;


// Test 1: Basic create and destroy
TEST(AVLTreeTest, BasicCreateDestroy)
{
    avl_h *avl = avl_create(nullptr);  // Create a new AVL tree
    ASSERT_NE(avl, nullptr);           // Ensure the AVL tree is created
    avl_destroy(avl);                  // Destroy the AVL tree
}

// Test 2: Insert one key and check that the key is inserted
TEST(AVLTreeTest, InsertSingleKey)
{
    avl_h *avl = avl_create(nullptr);  // Create the AVL tree

    std::string key = "key1";
    std::string data = "data1";
    int result = avl_insert(avl, key.c_str(), &data);

    ASSERT_EQ(result, 0);  // Ensure the insert was successful

    // Search for the key and verify that the data is correct
    void *retrieved_data = avl_search(avl, key.c_str());
    ASSERT_NE(retrieved_data, nullptr);  // Ensure the key is found
    ASSERT_EQ(retrieved_data, &data);    // Ensure the retrieved data matches

    avl_destroy(avl);  // Clean up
}

// Test 3: Insert multiple keys and check if the opaque data matches
TEST(AVLTreeTest, InsertMultipleKeys)
{
    avl_h *avl = avl_create(nullptr);  // Create the AVL tree

    std::string keys[10] = { "key1", "key2", "key3", "key4", "key5",
                             "key6", "key7", "key8", "key9", "key10" };
    std::string values[10] = { "data1", "data2", "data3", "data4", "data5",
                               "data6", "data7", "data8", "data9", "data10" };

    // Insert all keys and values
    for (int i = 0; i < 10; ++i) {
        int result = avl_insert(avl, keys[i].c_str(), &values[i]);
        ASSERT_EQ(result, 0);  // Check that insert returns success
    }

    // Check if all keys return the correct opaque values
    for (int i = 0; i < 10; ++i) {
        void *retrieved_data = avl_search(avl, keys[i].c_str());
        ASSERT_NE(retrieved_data, nullptr);  // Ensure the key is found
        ASSERT_EQ(retrieved_data, &values[i]);  // Ensure the data matches
    }

    avl_destroy(avl);  // Clean up
}


// Test 4: Delete a single key and check that it is removed
TEST(AVLTreeTest, DeleteSingleKey)
{
    avl_h *avl = avl_create(nullptr);  // Create the AVL tree

    std::string key = "key1";
    std::string data = "data1";
    avl_insert(avl, key.c_str(), &data);

    // Delete the key and check the returned data
    void *deleted_data = avl_del(avl, key.c_str());
    ASSERT_EQ(deleted_data, &data);  // Check that the correct data is returned

    // Ensure the key is no longer found
    void *search_data = avl_search(avl, key.c_str());
    ASSERT_EQ(search_data, nullptr);

    avl_destroy(avl);  // Clean up
}

// Test 5: Insert multiple keys, delete one and check that others are still present
TEST(AVLTreeTest, DeleteKeyAndCheckOthers)
{
    avl_h *avl = avl_create(nullptr);  // Create the AVL tree

    std::string keys[3] = { "key1", "key2", "key3" };
    std::string values[3] = { "data1", "data2", "data3" };

    for (int i = 0; i < 3; ++i) {
        avl_insert(avl, keys[i].c_str(), &values[i]);
    }

    // Delete key2 and check the returned data
    void *deleted_data = avl_del(avl, keys[1].c_str());  // Deleting "key2"
    ASSERT_EQ(deleted_data, &values[1]);

    // Ensure "key2" is no longer found
    void *search_data = avl_search(avl, keys[1].c_str());
    ASSERT_EQ(search_data, nullptr);

    // Ensure "key1" and "key3" are still present
    ASSERT_EQ(avl_search(avl, keys[0].c_str()), &values[0]);
    ASSERT_EQ(avl_search(avl, keys[2].c_str()), &values[2]);

    avl_destroy(avl);  // Clean up
}

// Test 6: Delete root in a tree with multiple keys
TEST(AVLTreeTest, DeleteRootKey)
{
    avl_h *avl = avl_create(nullptr);  // Create the AVL tree

    std::string keys[3] = { "key1", "key2", "key3" };
    std::string values[3] = { "data1", "data2", "data3" };

    // Insert the keys to create a tree with multiple levels
    for (int i = 0; i < 3; ++i) {
        avl_insert(avl, keys[i].c_str(), &values[i]);
    }

    // Delete the root key (which should be "key1" in a balanced tree)
    void *deleted_data = avl_del(avl, keys[0].c_str());
    ASSERT_EQ(deleted_data, &values[0]);

    // Ensure "key1" is no longer found
    void *search_data = avl_search(avl, keys[0].c_str());
    ASSERT_EQ(search_data, nullptr);

    // Ensure "key2" and "key3" are still present
    ASSERT_EQ(avl_search(avl, keys[1].c_str()), &values[1]);
    ASSERT_EQ(avl_search(avl, keys[2].c_str()), &values[2]);

    avl_destroy(avl);  // Clean up
}
