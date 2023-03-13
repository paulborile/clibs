#include <gtest/gtest.h>
#include <vector.h>

#include <fstream>
using namespace std;

// Simple test: creation, add, get, modify (set) and destroy a vector of int. Check on vector size after modify operation (del, add).
// Vector is allocated by library
TEST(V, basic_test)
{
    v_h *vh = NULL;
    int val[10];

    vh = v_create(vh, 10);
    ASSERT_NE((v_h *)0, vh);

    int size = v_len(vh);
    EXPECT_EQ(size, 0);

    for (int ind = 0; ind < 10; ind++)
    {
        val[ind] = 100 + ind;

        v_add(vh, &val[ind]);
    }

    size = v_len(vh);
    EXPECT_EQ(size, 10);

    for (int ind = 0; ind < 10; ind++)
    {
        int *num = (int *)v_get(vh, ind);
        EXPECT_EQ(100 + ind, *num);
    }

    v_delete(vh, 6);

    size = v_len(vh);
    EXPECT_EQ(size, 9);

    int *num = (int *)v_get(vh, 6);
    EXPECT_EQ(107, *num);

    int value = 210;
    v_set(vh, 6, &value);

    num = (int *)v_get(vh, 6);
    EXPECT_EQ(210, *num);

    int toadd = 300;
    v_add(vh, &toadd);

    size = v_len(vh);
    EXPECT_EQ(size, 10);

    v_destroy(vh);
}

// Resize test: create a vector with 30 elements. Then add more than 30 elements. Everything must be ok.
TEST(V, resize_test)
{
    v_h *vh = NULL;
    int val[40];

    vh = v_create(vh, 30);
    ASSERT_NE((v_h *)0, vh);

    int size = v_len(vh);
    EXPECT_EQ(size, 0);

    for (int ind = 0; ind < 40; ind++)
    {
        val[ind] = 100 + ind;

        v_add(vh, &val[ind]);
    }

    size = v_len(vh);
    EXPECT_EQ(size, 40);

    v_destroy(vh);
}
