#include <gtest/gtest.h>

#include "fh.h"

#include "CommandLineParams.h"

#include <fstream>
#include <pthread.h>
using namespace std;

fh_t *fhash = NULL;
fh_elem_t *savedata = NULL;

// trim from start
static inline std::string &ltrim(std::string &s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

// trim from end
static inline std::string &rtrim(std::string &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

// trim from both ends
std::string &trim(std::string &s)
{
    return ltrim(rtrim(s));
}

// This function get an enumerator, scan it and destroy it. Repeats this actions for numloop times. At the end, change value of execution flag,
// to stop other threads. DON'T USE THIS FUNCTION WITH del_and_insert().
void *scan_enumerator(void *param)
{
    int numloop = *((int *)param);
    cout << "Start thread scanning enumerator" << endl;

    for (int index = 0; index < numloop; index++)
    {
        int error = 0;

        fh_enum_t *fhe = fh_enum_create(fhash, FH_ENUM_SORTED_ASC, &error);
        if (fhe == NULL)
        {
            break;
        }

        while ( fh_enum_is_valid(fhe) )
        {
            fh_elem_t *element = fh_enum_get_value(fhe, &error);

            error = fh_enum_move_next(fhe);
            if (error == FH_BAD_HANDLE)
            {
                break;
            }
        }

        fh_enum_destroy(fhe);

//        cout << "Loop n° " << index + 1 << " completed!" << endl;
    }

    cout << "End thread scanning enumerator" << endl;

    return NULL;
}

// This function get an element and match it with saved copy. Count failure in match (not so useful)
void *get_and_check(void *param)
{
    int numloop = *((int *)param);
    cout << "Start get and check thread" << endl;
    int different = 0;

    for (int index = 0; index < numloop; index++)
    {
        // It's needed after first loop, I just want to know how much elements are modified in hashtable
        int attribute = 0;
        different = 0;

        // After introducing of insert that modifies hastable length, we have to know how long is savedata array to scan it,
        // not how much are elements in hashtable (that could be more than savedata ones).
        attribute = sizeof(savedata) / sizeof(savedata[0]);

        for (int ind = 0; ind < attribute; ind++)
        {
            int error = 0;

            char *value = (char *)fh_get(fhash, savedata[ind].key, &error);

            if (error < 0)
            {
                continue;
            }

            const char *obj = (const char *)savedata[ind].opaque_obj;
            if (strcmp(value, obj) != 0)
            {
                different++;
            }
        }
    }

    cout << "Get and check found " << different << " modified elements in hash" << endl;
    cout << "End get and check thread" << endl;

    return NULL;
}

// This function get a copy of an element and modify it.
void *search_and_modify(void *param)
{
    int numloop = *((int *)param);
    cout << "Start search and modify thread" << endl;
    int bufsize = 100;
    char value[bufsize];

    for (int index = 0; index < numloop; index++)
    {
        int attribute = 0;
        fh_getattr(fhash, FH_ATTR_ELEMENT, &attribute);

        for (int ind = 0; ind < attribute; ind++)
        {
            int error = 0;
            value[0] = '\0';

            error = fh_search(fhash, savedata[ind].key, value, bufsize);

            if (error < 0)
            {
                continue;
            }

            strcpy(value, "search and modify thread");
        }
    }

    cout << "End search and modify thread" << endl;

    return NULL;
}

// This function deletes all elements in hash table, then reinsert them using saved copy. At the end, change value of execution flag,
// to stop other threads. DON'T USE THIS FUNCTION WITH scan_enumerator().
void *del_and_insert(void *param)
{
    int numloop = *((int *)param);
    cout << "Start del and insert thread" << endl;

    int nelem = 0;
    fh_getattr(fhash, FH_ATTR_ELEMENT, &nelem);

    for (int index = 0; index < numloop; index++)
    {
        int attribute = 0;
        fh_getattr(fhash, FH_ATTR_ELEMENT, &attribute);

        if (attribute > 0)
        {
            for (int ind = 0; ind < nelem; ind++)
            {
                int error = fh_del(fhash, savedata[ind].key);

                if (error < 0)
                {
                    cout << "Delete error with key: " << savedata[ind].key << endl;
                }
            }

//           cout << "Deletion completed!" << endl;
        }
        else
        {
            for (int ind = 0; ind < nelem; ind++)
            {
                int error = fh_insert(fhash, savedata[ind].key, savedata[ind].opaque_obj);

                if (error < 0)
                {
                    cout << "Insert error with key: " << savedata[ind].key << endl;
                }
            }

//            cout << "Insertion completed!" << endl;
        }
    }

    cout << "End del and insert thread" << endl;

    return NULL;
}

// This function get an element and modify it.
void *searchlock_and_modify(void *param)
{
    int numloop = *((int *)param);
    cout << "Start searchlock and modify thread" << endl;

    for (int index = 0; index < numloop; index++)
    {
        int attribute = 0;
        fh_getattr(fhash, FH_ATTR_ELEMENT, &attribute);

        for (int ind = 0; ind < attribute; ind++)
        {
            int error = 0, pos = 0;

            char *value = (char *)fh_searchlock(fhash, savedata[ind].key, &pos, &error);

            if (error < 0)
            {
                continue;
            }

            if (strcmp((const char *)savedata[ind].opaque_obj, value) == 0)
            {
                int len = strlen(value);

                strncpy(value, "searchlock and modify thread", len);
            }
            else
            {
                strcpy(value, (char *)savedata[ind].opaque_obj);
            }

            error = fh_releaselock(fhash, pos);
        }
    }

    cout << "End searchlock and modify thread" << endl;

    return NULL;
}

// This function inserts n new elements in an existent hash table. This works with void pointer hash tables.
void *insert_only(void *param)
{
    int numloop = *((int *)param);
    cout << "Start insert only thread" << endl;

    for (int index = 0; index < numloop; index++)
    {
        char key[9];
        sprintf(key, "%08d", 100000 + index);
        char *value = (char *)malloc(100);
        strcpy(value, "Record number ");
        strcat(value, key);
        int error = fh_insert(fhash, key, value);

        if (error < 0)
        {
            cout << "Insert error with key: " << key << " and value " << value << endl;
        }
    }

    cout << "End insert only thread" << endl;

    return NULL;
}

// Check if search, searchlock, delete and insert functions should work at the same time
TEST(FH, multithread_test)
{
    char file[200];
    strcpy(file, CommandLineParamsParser::GetInstance()->GetValue("data-file").c_str());

    if (strlen(file) <= 0)
    {
        strcpy(file, "./data.txt");
    }

    cout << "Input file is " << file << endl;
    ifstream DataFile(file, std::ios::binary);
    ASSERT_TRUE(DataFile) << "Error: Cannot find " << CommandLineParamsParser::GetInstance()->GetValue("data-file").c_str();

    int error = 0;
    string line;

    // Create hash table of strings
    fhash = fh_create(100000, FH_DATALEN_STRING, NULL);
    ASSERT_NE((fh_t *)0, fhash);

    // Fill hashtable with data
    while (getline(DataFile, line))
    {
        /*
           line = trim(line);
           if(line.empty())
            continue;

           char *key = strtok((char *)line.c_str(), "\t");
           char *opaque = strtok(NULL, "\t");
         */

        fh_insert(fhash, (char *)line.c_str(), (void *)line.c_str());
    }

    DataFile.close();

    // Create a copy of data
    fh_enum_t *fhe = fh_enum_create(fhash, FH_ENUM_SORTED_ASC, &error);
    ASSERT_NE((fh_enum_t *)0, fhe);

    int size = fhe->size;
    int ind = 0;
    savedata = (fh_elem_t *)malloc(sizeof(fh_elem_t) * size);

    while ( fh_enum_is_valid(fhe) )
    {
        fh_elem_t *element = fh_enum_get_value(fhe, &error);
        char *key = (char *)malloc(strlen(element->key)+1);
        char *data = (char *)malloc(strlen((char *)element->opaque_obj)+1);

        strcpy(key, element->key);
        strcpy(data, (char *)element->opaque_obj);

        // Save each element for future comparisons
        savedata[ind].key = key;
        savedata[ind].opaque_obj = data;
        ind++;

        error = fh_enum_move_next(fhe);
        if (error == FH_BAD_HANDLE)
        {
            break;
        }
    }

    fh_enum_destroy(fhe);

    // Launch threads that work together on the same hash table
    int numexecsmall = 100;
    int numexec = 3000;
    pthread_t threads[3];
    void *retval;
    pthread_create(&threads[0], NULL, &del_and_insert, (void *)&numexecsmall);
    pthread_create(&threads[1], NULL, &search_and_modify, (void *)&numexec);
    pthread_create(&threads[2], NULL, &searchlock_and_modify, (void *)&numexec);
    pthread_join(threads[0], &retval);
    pthread_join(threads[1], &retval);
    pthread_join(threads[2], &retval);

    // Destroy hash table
    fh_destroy(fhash);

    // Destroy savedata
    for (int ind = 0; ind < size; ind++)
    {
        if (savedata[ind].key != NULL)
        {
            free(savedata[ind].key);
        }
        if (savedata[ind].opaque_obj != NULL)
        {
            free(savedata[ind].opaque_obj);
        }
    }

    free(savedata);
}

// Check if search, searchlock and enumerator functions should work at the same time
TEST(FH, multithread_test_with_enum)
{
    char file[200];
    strcpy(file, CommandLineParamsParser::GetInstance()->GetValue("data-file").c_str());

    if (strlen(file) <= 0)
    {
        strcpy(file, "./data.txt");
    }

    cout << "Input file is " << file << endl;
    ifstream DataFile(file, std::ios::binary);
    ASSERT_TRUE(DataFile) << "Error: Cannot find " << CommandLineParamsParser::GetInstance()->GetValue("data-file").c_str();

    int error = 0;
    string line;

    // Create hash table of strings
    fhash = fh_create(100000, FH_DATALEN_STRING, NULL);
    ASSERT_NE((fh_t *)0, fhash);

    // Fill hashtable with data
    while (getline(DataFile, line))
    {
        /*
           line = trim(line);
           if(line.empty())
            continue;

           char *key = strtok((char *)line.c_str(), "\t");
           char *opaque = strtok(NULL, "\t");
         */

        fh_insert(fhash, (char *)line.c_str(), (void *)line.c_str());
    }

    DataFile.close();

    // Create a copy of data
    fh_enum_t *fhe = fh_enum_create(fhash, FH_ENUM_SORTED_ASC, &error);
    ASSERT_NE((fh_enum_t *)0, fhe);

    int size = fhe->size;
    int ind = 0;
    savedata = (fh_elem_t *)malloc(sizeof(fh_elem_t) * size);

    while ( fh_enum_is_valid(fhe) )
    {
        fh_elem_t *element = fh_enum_get_value(fhe, &error);
        char *key = (char *)malloc(strlen(element->key)+1);
        char *data = (char *)malloc(strlen((char *)element->opaque_obj)+1);

        strcpy(key, element->key);
        strcpy(data, (char *)element->opaque_obj);

        // Save each element for future comparisons
        savedata[ind].key = key;
        savedata[ind].opaque_obj = data;
        ind++;

        error = fh_enum_move_next(fhe);
        if (error == FH_BAD_HANDLE)
        {
            break;
        }
    }

    fh_enum_destroy(fhe);

    // Launch threads that work together on the same hash table. First of all, set execution to true (was set to false at the end of previous test)
    int numexecsmall = 100;
    int numexec = 200;
    pthread_t threads[3];
    void *retval;
    pthread_create(&threads[0], NULL, &scan_enumerator, (void *)&numexecsmall);
    pthread_create(&threads[1], NULL, &search_and_modify, (void *)&numexec);
    pthread_create(&threads[2], NULL, &searchlock_and_modify, (void *)&numexec);
    pthread_join(threads[0], &retval);
    pthread_join(threads[1], &retval);
    pthread_join(threads[2], &retval);

    // Destroy hash table
    fh_destroy(fhash);

    // Destroy savedata
    for (int ind = 0; ind < size; ind++)
    {
        if (savedata[ind].key != NULL)
        {
            free(savedata[ind].key);
        }
        if (savedata[ind].opaque_obj != NULL)
        {
            free(savedata[ind].opaque_obj);
        }
    }

    free(savedata);
}

// Check if get and enumerator functions should work at the same time with void pointer hashtable
TEST(FH, DISABLED_multithread_test_with_void_pointer)
{
    char file[200];
    strcpy(file, CommandLineParamsParser::GetInstance()->GetValue("data-file").c_str());

    if (strlen(file) <= 0)
    {
        strcpy(file, "../resources/fh-data.tsv");
    }

    cout << "Input file is " << file << endl;
    ifstream DataFile(file, std::ios::binary);
    ASSERT_TRUE(DataFile) << "Error: Cannot find " << CommandLineParamsParser::GetInstance()->GetValue("data-file").c_str();

    int error = 0;
    string line;

    // Create hash table of strings
    fhash = fh_create(100000, FH_DATALEN_VOIDP, NULL);
    ASSERT_NE((fh_t *)0, fhash);

    // Fill hashtable with data
    while (getline(DataFile, line))
    {
        line = trim(line);
        if (line.empty())
            continue;

        char *key = strtok((char *)line.c_str(), "\t");
        char *value = strtok(NULL, "\t");

        char *opaque = (char *)malloc(100);
        strcpy(opaque, value);

        fh_insert(fhash, key, opaque);
    }

    DataFile.close();

    // Create a copy of data
    fh_enum_t *fhe = fh_enum_create(fhash, FH_ENUM_SORTED_ASC, &error);
    ASSERT_NE((fh_enum_t *)0, fhe);

    int size = fhe->size;
    int ind = 0;
    savedata = (fh_elem_t *)malloc(sizeof(fh_elem_t) * size);

    while ( fh_enum_is_valid(fhe) )
    {
        fh_elem_t *element = fh_enum_get_value(fhe, &error);
        char *key = (char *)malloc(10);

        strcpy(key, element->key);

        // Save each element for future comparisons
        savedata[ind].key = key;
        savedata[ind].opaque_obj = element->opaque_obj;
        ind++;

        error = fh_enum_move_next(fhe);
        if (error == FH_BAD_HANDLE)
        {
            break;
        }
    }

    fh_enum_destroy(fhe);

    // Launch threads that work together on the same hash table. First of all, set execution to true (was set to false at the end of previous test)
    int numexec = 10000;
    pthread_t threads[2];
    void *retval;
    pthread_create(&threads[0], NULL, &insert_only, (void *)&numexec);
    pthread_create(&threads[1], NULL, &get_and_check, (void *)&numexec);
    pthread_join(threads[0], &retval);
    pthread_join(threads[1], &retval);

    // Destroy hash table
    fh_clean(fhash, free);
    fh_destroy(fhash);

    // Destroy savedata
    for (int ind = 0; ind < size; ind++)
    {
        if (savedata[ind].key != NULL)
        {
            free(savedata[ind].key);
        }
    }

    free(savedata);
}
