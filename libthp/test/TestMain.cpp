#include <gtest/gtest.h>
#include <fstream>
using namespace std;


int main(int argc, char *argv[])
{
    cout << "Running main() in file c-libraries/libfh/test/TestMain.cpp" << endl;
    cout << "Now running test executable: " << argv[0] << endl;

    testing::InitGoogleTest(&argc, argv);

    int testResult = ::testing::UnitTest::GetInstance()->Run();
    return testResult;
}
