#include <gtest/gtest.h>


#include "thp.h"

#include <fstream>
using namespace std;


// basic create destroy test
TEST(THP, basic_test)
{
    thp_h *t = thp_create(NULL, 4);

    thp_destroy(t);
}
