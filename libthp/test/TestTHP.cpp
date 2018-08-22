#include <gtest/gtest.h>


#include "thp.h"

#include <fstream>
using namespace std;


// basic create destroy test
TEST(THP, basic_test)
{
    int err;
    thp_h *t = thp_create(NULL, 1, &err);
    EXPECT_NE(0, t);

    thp_destroy(t);
}

long counter = 0;
static void *adder(void *v)
{
    long inc = (long) v;
    counter += inc;
    //printf("adder executing with %d, counter %d\n", inc, counter);
    return 0;
}

TEST(THP, ThreadRunNoParam)
{
    int err;

    thp_h *t = thp_create(NULL, 1, &err);
    EXPECT_NE(0, t);

    counter = 0;
    for (int i = 0; i <100; i++)
    {
        thp_add(t, adder, (void *)1);
    }
    EXPECT_EQ(100, thp_wait(t));
    EXPECT_EQ(100, counter);
    thp_destroy(t);
}

TEST(THP, ThreadRunWithParam)
{
    int err;
    int max = 10;

    thp_h *t = thp_create(NULL, 1, &err);
    EXPECT_NE(0, t);

    counter = 0;
    for (int i = 1; i <=max; i++)
    {
        thp_add(t, adder, (void *)i);
    }
    EXPECT_EQ(max, thp_wait(t));
    // thanks Gauss
    EXPECT_EQ(max*(max+1)/2, counter);
    thp_destroy(t);
}
