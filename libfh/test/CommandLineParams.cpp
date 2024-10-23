/*  System include files                        */
#include <iostream>
#include <fstream>
#include <cassert>
#include <numeric>

/*  Project include files                       */
/*  Module include files                        */
/*  Local constants                             */
/*  Local types                                 */
/*  Local macros                                */

using namespace std;

#include "CommandLineParams.h"

CommandLineParamsParser::CommandLineParamsParser()
{
}

bool CommandLineParamsParser::AddParam(const string& o, const string& d, CommandLineParamType t)
{
    theInstance->paramsMap.insert(make_pair(o, CommandLineParam(o, d, t)));
    return true;
}

void CommandLineParamsParser::PrintHelp()
{
    cout << "\nParams Descriptions:" << endl;
    for (map<string, CommandLineParam>::iterator i = theInstance->paramsMap.begin(); i != theInstance->paramsMap.end(); i++)
    {
        cout << "\t--" << i->first << " : " << i->second.description << " (" << (i->second.type == Mandatory ? "Mandatory" : "Optional") << ")" << endl;
    }

    cout << "\t-h, --help : Get this help and exit" << endl;
}


void CommandLineParamsParser::PrintValues()
{
    cout << "\nParams Values:" << endl;

    for (map<string, CommandLineParam>::iterator i = theInstance->paramsMap.begin(); i != theInstance->paramsMap.end(); i++)
    {
        cout << "\t--" << i->first << " = " << i->second.value << endl;
    }
}

string CommandLineParamsParser::GetValue(const string& key)
{
    map<string, CommandLineParam>::iterator i = theInstance->paramsMap.find(key);
    if (i != theInstance->paramsMap.end())
    {
        return i->second.value;
    }

    // must not hit here. You are trying to use an undefined parameter.
    cout << "Unknown parameter: <" << key << ">" << endl;
    assert(0);
    return "";
}

bool CommandLineParamsParser::Parse(int argc, char **argv, bool reportErrors)
{
    // parse params
    for (int i = 1; i < argc; i++)
    {
        string paramOption(argv[i]);
        if (paramOption == "--help" || paramOption == "-h")
        {
            PrintHelp();
            return false;
        }
        else
        {
            // remove trailing '-'s
            int dashCount = 0;
            while (paramOption[0] == '-')
            {
                paramOption = paramOption.substr(1);
                dashCount++;
            }

            if (theInstance->paramsMap.count(paramOption))
            {
                theInstance->paramsMap.at(paramOption).value = argv[i + 1];
                i++;
            }
            else if (reportErrors)
            {
                cerr << "Unknown " << (dashCount > 0 ? "option" : "token" ) << " : " << string(dashCount, '-') << paramOption << endl;
                cerr << "Please SEPARATE options names and values with at least one space" << endl;
                PrintHelp();
                return false;
            }
        }
    }

    // validate
    bool mustExit = false;
    for (map<string, CommandLineParam>::iterator i = theInstance->paramsMap.begin(); i != theInstance->paramsMap.end(); i++)
    {
        if (i->second.type == Mandatory && i->second.value.empty())
        {
            cerr << "Option --" << i->first << " is MANDATORY. Please provide a value." << endl;
            mustExit = true;
        }
    }

    // PrintValues();

    if (mustExit)
    {
        PrintHelp();
        return false;
    }

    return true;
}

CommandLineParamsParser::~CommandLineParamsParser()
{
    if (theInstance)
    {
        delete theInstance;
    }
}

CommandLineParamsParser *CommandLineParamsParser::theInstance = NULL;

CommandLineParamsParser *CommandLineParamsParser::GetInstance()
{
    if (!theInstance)
    {
        theInstance = new CommandLineParamsParser;
    }

    return theInstance;
}
