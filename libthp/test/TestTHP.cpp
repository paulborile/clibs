#include <gtest/gtest.h>


#include "thp.h"

#include <fstream>
using namespace std;


// basic create destroy test
TEST(THP, basic_test)
{
    int err;
    thp_h *t = thp_create(NULL, 4, &err);
    EXPECT_NE(0, t);

    thp_destroy(t);
}

int counter = 0;
static void adder(void *v)
{
    counter++;
}

static void subber(void *v)
{
    counter--;
}

TEST(THP, ThreadRun)
{
    int err;

    thp_h *t = thp_create(NULL, 2, &err);
    EXPECT_NE(0, t);

    counter = 0;
    for (int i = 0; i <100; i++)
    {
        thp_add(t, adder);
    }
    thp_wait(t);
    EXPECT_EQ(100, counter);
    thp_destroy(t);
}
