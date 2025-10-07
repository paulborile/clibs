#include <gtest/gtest.h>
#include <time.h>


#include "timing.h"
#include "fh.h"
#include "thp.h"

#include "CommandLineParams.h"

#include <algorithm> // for std::find_if
#include <cctype>    // for std::isspace
#include <fstream>
#include <pthread.h>
#include <thread>
#include <chrono>

using namespace std;

fh_t *fhash = NULL;
fh_elem_t *savedata = NULL;

// trim from start
static inline std::string &ltrim(std::string &s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    return s;
}

// trim from end
static inline std::string &rtrim(std::string &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
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
    int error;
    int pos;
    void *opaque_obj_ptr = NULL;


    int nelem = 0;
    fh_getattr(fhash, FH_ATTR_ELEMENT, &nelem);
    cout << "Elements in hash before deletion: " << nelem << endl;

    for (int index = 0; index < numloop; index++)
    {
        int attribute = 0;
        fh_getattr(fhash, FH_ATTR_ELEMENT, &attribute);

        if (attribute > 0)
        {
            for (int ind = 0; ind < nelem; ind++)
            {
                error = fh_del(fhash, savedata[ind].key);

                if (error < 0)
                {
                    cout << "Delete error with key: " << savedata[ind].key << endl;
                }
            }

            cout << "Deletion completed!" << endl;
        }
        else
        {
            for (int ind = 0; ind < nelem; ind++)
            {
                // every pair element use fh_insertlock instead of fh_insert
                if (ind % 2 == 0)
                {
                    char *value = (char *) fh_insertlock(fhash, savedata[ind].key, savedata[ind].opaque_obj, &pos, &error, &opaque_obj_ptr);
                    // do something to waste some time before release lock
                    struct timespec t;
                    t.tv_sec = 0;
                    t.tv_nsec = 1000; // 1 micro = 1,000 nanoseconds

                    nanosleep(&t, NULL);

                    if (error == FH_OK)
                    {
                        fh_releaselock(fhash, pos);
                    }
                }
                else
                {
                    int error = fh_insert(fhash, savedata[ind].key, savedata[ind].opaque_obj);
                }

                if (error < 0)
                {
                    cout << "Insert error with key: " << savedata[ind].key << endl;
                }
            }

            cout << "Insertion completed!" << endl;
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
        char key[16];
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
    int numexecsmall = 5;
    int numexec = 30000;
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
TEST(FH, multithread_test_with_void_pointer)
{
    char file[200];
    strcpy(file, CommandLineParamsParser::GetInstance()->GetValue("data-file").c_str());

    if (strlen(file) <= 0)
    {
        strcpy(file, "./fh-data.tsv");
    }

    cout << "Input file is " << file << endl;
    ifstream DataFile(file, std::ios::binary);
    ASSERT_TRUE(DataFile) << "Error: Cannot find " << CommandLineParamsParser::GetInstance()->GetValue("data-file").c_str();

    int error = 0;
    string line;

    // Create hash table of strings
    fhash = fh_create(10000, FH_DATALEN_VOIDP, NULL);
    ASSERT_NE((fh_t *)0, fhash);

    // Fill hashtable with data
    while (getline(DataFile, line))
    {
        line = trim(line);
        if (line.empty())
        {
            continue;
        }

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
    int numexec = 100000;
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

#define MAX_THREADS 8
struct thread_stats
{
    int64_t fh_get_time;
    double avg_time;
    void *t; // for timing_new_timer(TIMING_NANOPRECISION);

};
struct thread_data
{
    int fh_size;
    fh_t *f;
    int thread_num;
    int *rand_nums;
    int cycles; // how many times we want to cycle on random values
    struct thread_stats *tstat;
};

extern double  compute_average(double current_avg, int count, int new_value);

// measure_fh_get_speed_thread
void *measure_fh_get_speed_thread(void *v)
{
    struct thread_data *td = (struct thread_data *) v;
    int i = td->cycles;
    double delta;
    int error;
    int count = 0;

    while (i--) {
        // generate random string
        for (int j = 0; j < td->fh_size; j++)
        {
            int rand_place = 0;
            if (j % 3 == 0)
            {
                // once every 3 items go completely casual
                rand_place = rand() % td->fh_size;
            }
            else
            {
                rand_place = td->rand_nums[j];
            }

            string key = "key" + to_string(rand_place);
            timing_start(td->tstat[td->thread_num].t);
            char *value = (char *) fh_get(td->f, (char *) key.c_str(), &error);
            delta = timing_end(td->tstat[td->thread_num].t);
            if (error == FH_OK)
            {
                // test for correctness of value
                string value_str = "value" + to_string(rand_place);
                if (strcmp(value, value_str.c_str()) != 0)
                {
                    printf("Error: value for key %s is %s, should be %s\n", key.c_str(), value, value_str.c_str());
                    return NULL;
                }
            }
            td->tstat[td->thread_num].fh_get_time += delta;
            td->tstat[td->thread_num].avg_time = compute_average(td->tstat[td->thread_num].avg_time, count++, delta);
        }
    }
    free(v); // free the copy generated for this thread.
    return NULL;
}

// a test to measure concurrent fh_get() speed with different number of threads
TEST(FH, MultithreadGetSpeedMutex)
{
    for (int k = 1; k <= MAX_THREADS; k++)
    {
        int err;
        struct thread_data tdata = {0};
        const int fh_size = 400000;
        const int thp_size = k;

        fh_t *fh = fh_create(fh_size, FH_DATALEN_STRING, NULL);
        ASSERT_NE(nullptr, fh);
        tdata.rand_nums = (int *)malloc(sizeof(int) * fh_size);
        tdata.cycles = 10;

        // now fill the hash table with random keys and values that copy the key
        srand(0); // we want to be ablet to reproduce the results
        for (int i = 0; i < fh_size; i++)
        {
            int rand_num = rand();
            tdata.rand_nums[i] = rand_num;
            string key = "key" + to_string(rand_num);
            string value = "value" + to_string(rand_num);
            fh_insert(fh, (char *) key.c_str(), (void *) value.c_str());
        }
        // create a thread pool with libthp
        thp_h *thp = thp_create(NULL, thp_size, &err);
        ASSERT_NE(nullptr, thp);

        tdata.f = fh;
        tdata.fh_size = fh_size;
        tdata.tstat = (struct thread_stats *)calloc(thp_size, sizeof(struct thread_stats));

        // now run threads that will concurrently call fh_get()
        for (int i = 0; i < thp_size; i++)
        {
            struct thread_data *ptd = (struct thread_data *) malloc(sizeof(struct thread_data));
            *ptd = tdata;
            ptd->tstat[i].t = timing_new_timer(TIMING_NANOPRECISION);
            ptd->thread_num = i;
            thp_add(thp, measure_fh_get_speed_thread, (void *) ptd);
        }

        // wait for all threads to finish
        thp_wait(thp);
        free(tdata.rand_nums);

        // print statistics of avg time per fh_get() call
        // for (int i = 0; i < thp_size; i++)
        // {
        //     printf("Thread %d: %.2f\n", i, tdata.tstat[i].avg_time);
        // }

        // compute cumulative calls per second
        double total_fhget_sec = 0;
        for (int i = 0; i < thp_size; i++)
        {
            total_fhget_sec += (double) 1.0 / (tdata.tstat[i].fh_get_time/1000000000.0)*(double) (fh_size*tdata.cycles);
        }

        printf("%d threads - Total calls per second: %.2f\n", thp_size, total_fhget_sec);

        // free memory
        for (int i = 0; i < thp_size; i++)
        {
            timing_delete_timer(tdata.tstat[i].t);
        }
        free(tdata.tstat);
        fh_destroy(tdata.f);
        thp_destroy(thp);
    }

}

// a test to measure concurrent fh_get() speed with different number of threads
TEST(FH, MultithreadGetSpeedRWLocks)
{
    for (int k = 1; k <= MAX_THREADS; k++)
    {
        int err;
        struct thread_data tdata = {0};
        const int fh_size = 400000;
        const int thp_size = k;

        fh_t *fh = fh_create(fh_size, FH_DATALEN_STRING, NULL);
        ASSERT_NE(nullptr, fh);
        fh_setattr(fh, FH_SETATTR_USERWLOCKS, 1);
        tdata.rand_nums = (int *)malloc(sizeof(int) * fh_size);
        tdata.cycles = 10;

        // now fill the hash table with random keys and values that copy the key
        srand(0); // we want to be ablet to reproduce the results
        for (int i = 0; i < fh_size; i++)
        {
            int rand_num = rand();
            tdata.rand_nums[i] = rand_num;
            string key = "key" + to_string(rand_num);
            string value = "value" + to_string(rand_num);
            fh_insert(fh, (char *) key.c_str(), (void *) value.c_str());
        }
        // create a thread pool with libthp
        thp_h *thp = thp_create(NULL, thp_size, &err);
        ASSERT_NE(nullptr, thp);

        tdata.f = fh;
        tdata.fh_size = fh_size;
        tdata.tstat = (struct thread_stats *)calloc(thp_size, sizeof(struct thread_stats));

        // now run threads that will concurrently call fh_get()
        for (int i = 0; i < thp_size; i++)
        {
            struct thread_data *ptd = (struct thread_data *) malloc(sizeof(struct thread_data));
            *ptd = tdata;
            ptd->tstat[i].t = timing_new_timer(TIMING_NANOPRECISION);
            ptd->thread_num = i;
            thp_add(thp, measure_fh_get_speed_thread, (void *) ptd);
        }

        // wait for all threads to finish
        thp_wait(thp);
        free(tdata.rand_nums);

        // print statistics of avg time per fh_get() call
        // for (int i = 0; i < thp_size; i++)
        // {
        //     printf("Thread %d: %.2f\n", i, tdata.tstat[i].avg_time);
        // }

        // compute cumulative calls per second
        double total_fhget_sec = 0;
        for (int i = 0; i < thp_size; i++)
        {
            total_fhget_sec += (double) 1.0 / (tdata.tstat[i].fh_get_time/1000000000.0)*(double) (fh_size*tdata.cycles);
        }

        printf("%d threads - Total calls per second: %.2f\n", thp_size, total_fhget_sec);

        // free memory
        for (int i = 0; i < thp_size; i++)
        {
            timing_delete_timer(tdata.tstat[i].t);
        }
        free(tdata.tstat);
        fh_destroy(tdata.f);
        thp_destroy(thp);
    }

}

// measure_fh_get_insert_del_speed_thread
void *measure_fh_get_insert_del_speed_thread(void *v)
{
    struct thread_data *td = (struct thread_data *) v;
    int i = td->cycles;
    double delta;
    int error;
    int count = 0;

    while (i--) {
        // generate random string
        for (int j = 0; j < td->fh_size; j++)
        {
            int rand_place = 0;
            // if (j % 3 == 0)
            // {
            //     // once every 3 items go completely casual
            //     rand_place = rand() % td->fh_size;
            // }
            // else
            // {
            rand_place = td->rand_nums[j];
            // }

            string key = "key" + to_string(rand_place);

            timing_start(td->tstat[td->thread_num].t);
            char value[64];
            error = fh_search(td->f, (char *) key.c_str(), value, 64);
            // only 20% of the time simulate a cache miss
            if (rand_place % 5 )
            {
                fh_del(td->f, (char *) key.c_str());
                string ins_str = "value" + to_string(rand_place);
                fh_insert(td->f, (char *) key.c_str(), (void *) ins_str.c_str());
            }
            delta = timing_end(td->tstat[td->thread_num].t);

            if (error >= 0)
            {
                // test for correctness of value
                string value_str = "value" + to_string(rand_place);
                if (strcmp(value, value_str.c_str()) != 0)
                {
                    printf("Error: value for key %s is %s, should be %s\n", key.c_str(), value, value_str.c_str());
                    return NULL;
                }
            }
            td->tstat[td->thread_num].fh_get_time += delta;
            td->tstat[td->thread_num].avg_time = compute_average(td->tstat[td->thread_num].avg_time, count++, delta);
        }
    }
    free(v); // free the copy generated for this thread.
    return NULL;
}

// a test to measure tipical lru cache use :
// - get only threads
// - get -> insert -> delete threads
TEST(FH, MultithreadGetInsertDelSpeed)
{
    int MaxThreads = 8;
    pthread_t threads[MaxThreads];
    void *retval;

    for (int k = 1; k <= 8; k++)
    {
        int err;
        struct thread_data tdata = {0};
        const int fh_size = 400000;
        const int thp_size = k;

        fh_t *fh = fh_create(fh_size, FH_DATALEN_STRING, NULL);
        ASSERT_NE(nullptr, fh);
        tdata.rand_nums = (int *)malloc(sizeof(int) * fh_size);
        tdata.cycles = 10;

        // now fill the hash table with random keys and values that copy the key
        srand(0); // we want to be ablet to reproduce the results
        for (int i = 0; i < fh_size; i++)
        {
            int rand_num = rand();
            tdata.rand_nums[i] = rand_num;
            string key = "key" + to_string(rand_num);
            string value = "value" + to_string(rand_num);
            fh_insert(fh, (char *) key.c_str(), (void *) value.c_str());
        }
        // create a thread pool with libthp
        // thp_h *thp = thp_create(NULL, thp_size, &err);
        // ASSERT_NE(nullptr, thp);

        tdata.f = fh;
        tdata.fh_size = fh_size;
        tdata.tstat = (struct thread_stats *)calloc(thp_size, sizeof(struct thread_stats));

        // now run threads that will concurrently call fh_get()
        for (int i = 0; i < thp_size; i++)
        {
            struct thread_data *ptd = (struct thread_data *) malloc(sizeof(struct thread_data));
            *ptd = tdata;
            ptd->tstat[i].t = timing_new_timer(TIMING_NANOPRECISION);
            ptd->thread_num = i;
            // thp_add(thp, measure_fh_get_insert_del_speed_thread, (void *) ptd);
            pthread_create(&threads[i], NULL, measure_fh_get_insert_del_speed_thread, (void *) ptd);
        }

        // wait for all threads to finish
        // thp_wait(thp);
        for (int i = 0; i < thp_size; i++)
        {
            pthread_join(threads[i], &retval);
        }

        // print statistics of avg time per fh_get() call
        // for (int i = 0; i < thp_size; i++)
        // {
        //     printf("Thread %d: %.2f\n", i, tdata.tstat[i].avg_time);
        // }

        // compute cumulative calls per second
        double total_fhget_sec = 0;
        for (int i = 0; i < thp_size; i++)
        {
            total_fhget_sec += (double) 1.0 / (tdata.tstat[i].fh_get_time/1000000000.0)*(double) (fh_size*tdata.cycles);
        }
        int elements;
        fh_getattr(tdata.f, FH_ATTR_ELEMENT, &elements);
        printf("%d threads - Total calls per second: %.2f, elements in hash %d\n", thp_size, total_fhget_sec, elements);

        // free memory
        for (int i = 0; i < thp_size; i++)
        {
            timing_delete_timer(tdata.tstat[i].t);
        }
        // thp_destroy(thp);
        fh_destroy(tdata.f);
        free(tdata.rand_nums);
        free(tdata.tstat);

    }
}

double  compute_average(double current_avg, int count, int new_value)
{
    if ( count == 0 )
    {
        return (new_value);
    }
    else
    {
        return (((current_avg * count) + new_value) / (count+1));
    }
}
