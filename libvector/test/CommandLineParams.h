/*
 * 
 * CommandLineParams.h
 *
 * Project : WURFL-InFuze Google Test Suite
 *
 * Module: 
 *
 * Contents: 
 *
 * Author(s): aaron
 *
 * Date: Jul 22, 2016
 * 
 * Copyright (c) ScientiaMobile, Inc. 2015-2016
 *
 */

#ifndef COMMANDLINEPARAMS_H_
#define COMMANDLINEPARAMS_H_


// #include <wurfl/wurfl.h>

/*  System include files                        */
#include <string>
#include <map>
using namespace std;
/*  Project include files                       */
/*  Module include files                        */
/*  Local constants                             */
/*  Local types                                 */
/*  Local macros                                */

enum CommandLineParamType
{
    Mandatory,
    Optional
};

struct CommandLineParam
{
    string option;
    string description;
    string value;
    CommandLineParamType type;
    CommandLineParam(const string& o, const string& d, CommandLineParamType t) : option(o), description(d), type(t) {}

};

class CommandLineParamsParser
{
public:
    CommandLineParamsParser();
    virtual ~CommandLineParamsParser();
    static CommandLineParamsParser* GetInstance();
    bool Parse(int argc, char** argv, bool reportErrors = true);
    bool AddParam(const string& o, const string& d, CommandLineParamType t);
    void PrintHelp();
    void PrintValues();
    string GetValue(const string& key);
private:
    static CommandLineParamsParser* theInstance;
    map<string, CommandLineParam> paramsMap;
};


// #define UPDATER_URL "https://data.scientiamobile.com/iqfad/wurfl.zip"
#define UPDATER_URL "https://data.scientiamobile.com/wjdxg/daily/wurfl.zip"


#endif /* COMMANDLINEPARAMS_H_ */
