/*
 *
 * Project : Vector library
 *
 * Copyright (c) ScientiaMobile, Inc. 2016
 *
 */

#include <iostream>
#include <fstream>
using namespace std;

#include <gtest/gtest.h>

#include "CommandLineParams.h"

int main(int argc, char *argv[])
{
    cout << "Running main() in file c-libraries/libvector/test/TestMain.cpp" << endl;
    cout << "Now running test executable: " << argv[0] << endl;

    CommandLineParamsParser::GetInstance()->AddParam("ua-file", "Real traffic UA file for speed test", CommandLineParamType::Optional);

    testing::InitGoogleTest(&argc, argv);

    // re-parse, now with full error control, to check for wrong stuff on command line
    bool extraParamsParsed = CommandLineParamsParser::GetInstance()->Parse(argc, argv);
    if (!extraParamsParsed)
    {
        return -1;
    }

    CommandLineParamsParser::GetInstance()->PrintValues();

    // check for data files existence
    {
        string dataFile = CommandLineParamsParser::GetInstance()->GetValue("ua-file");
        if ( dataFile != "")
        {
            std::ifstream fullFile(dataFile, std::ios::binary);
            if (!fullFile)
            {
                cout << "Cannot open <" << dataFile << "> file." << endl;
                return -1;
            }
            fullFile.close();
        }
    }

    int testResult = ::testing::UnitTest::GetInstance()->Run();
    return testResult;
}
