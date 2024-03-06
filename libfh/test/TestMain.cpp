
#define PICOBENCH_IMPLEMENT
#include <picobench/picobench.hpp>
#include <gtest/gtest.h>
#include <fstream>
using namespace std;

#include "CommandLineParams.h"


int main(int argc, char *argv[])
{
    cout << "Running main() in file c-libraries/libfh/test/TestMain.cpp" << endl;
    cout << "Now running test executable: " << argv[0] << endl;

    CommandLineParamsParser::GetInstance()->AddParam("data-file", "Data file", Optional);

    testing::InitGoogleTest(&argc, argv);

    // re-parse, now with full error control, to check for wrong stuff on command line
    bool extraParamsParsed = CommandLineParamsParser::GetInstance()->Parse(argc, argv);
    /*
       if (!extraParamsParsed)
       {
        // run anyway the google test suite, in case I am specifying some google test switch like --gtest_list_tests
        return ::testing::UnitTest::GetInstance()->Run();
       }
     */

//    CommandLineParamsParser::GetInstance()->PrintValues();
//
//    {
//        std::ifstream uaFile(CommandLineParamsParser::GetInstance()->GetValue("ua-file").c_str(), std::ios::binary);
//        if (!uaFile)
//        {
//            cout << "Cannot open UA file" << endl;
//            return -1;
//        }
//        uaFile.close();
//    }

    int testResult = ::testing::UnitTest::GetInstance()->Run();

    // For picobench

    picobench::runner r;
    // Optionally parse command line
    // r.parse_cmd_line(argc, argv);
    // r.run();

    return testResult;
}
