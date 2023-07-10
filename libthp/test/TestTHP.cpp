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

pthread_mutex_t counter_mutex;
long counter = 0;
static void *adder(void *v)
{
    long inc = (long) v;
    pthread_mutex_lock(&counter_mutex);
    counter += inc;
    pthread_mutex_unlock(&counter_mutex);
    //printf("adder executing with %d, counter %d\n", inc, counter);
    return 0;
}
#define NUM_THREADS 6
#define NUM_JOBS 100000

TEST(THP, ThreadRunNoParam)
{
    int err;

    pthread_mutex_init(&counter_mutex, NULL);

    thp_h *t = thp_create(NULL, NUM_THREADS, &err);
    EXPECT_NE(0, t);

    counter = 0;
    for (int i = 0; i <NUM_JOBS; i++)
    {
        thp_add(t, adder, (void *)1);
    }
    EXPECT_EQ(NUM_JOBS, thp_wait(t));
    EXPECT_EQ(NUM_JOBS, counter);
    thp_destroy(t);
}

TEST(THP, ThreadRunWithParam)
{
    int err;
    long max = NUM_JOBS;

    pthread_mutex_init(&counter_mutex, NULL);

    thp_h *t = thp_create(NULL, NUM_THREADS, &err);
    EXPECT_NE(0, t);

    counter = 0;
    for (long i = 1; i <=max; i++)
    {
        thp_add(t, adder, (void *)i);
    }
    EXPECT_EQ(max, thp_wait(t));
    // thanks Gauss
    long total = max*(max+1)/2;
    EXPECT_EQ(total, counter);
    thp_destroy(t);
}
