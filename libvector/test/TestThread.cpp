#include <gtest/gtest.h>

#include <fstream>
#include <thread>
#include <chrono>

#include <vector.h>
#include "CommandLineParams.h"

using namespace std;

typedef struct _th_params
{
    int num;
    string uaFile;
    v_h *vh;
} th_params;

int thread_results [8][2] =
{
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
};

// each thread will read strings from a file (same file for all threads)
// performs insert adding his own signature
void *test_add_get(void *params)
{
    th_params *data = (th_params *)params;
    cout << "Start of thread add_get n. " << data->num << endl;

    // the thread signature prepended to inserting string
    string thread_signature = to_string(data->num) + "___";

    // the number of elements inserted by this thread
    int inserted_elements = 0;

    // Take UAs from a file
    ifstream uaStream(data->uaFile, ios::binary);

    if(!uaStream)
    {
        cout << "Cannot find " << data->uaFile << " UA file" << endl;
        return NULL;
    }

    string line;

    while (getline(uaStream, line) /* && lineCount < 100 */ )
    {
        //       line = trim(line);
        if(line.empty())
            continue;

        line = thread_signature + line;
        v_add(data->vh, (void *)strdup(line.c_str()));
        inserted_elements++;
    }

    // vector length should be at least equal to elements inserted by the thread
    int vector_length = v_len(data->vh);
    EXPECT_GE(vector_length, inserted_elements);

    // check if all inserted elements still present in vector
    int found_elements = 0;
    for (int i = 0; i < vector_length; i++)
    {
        string val((char *)v_get(data->vh, i));
        if (val.rfind(thread_signature, 0) == 0)
        {
            found_elements++;
        }
    }

    // store thread results
    thread_results[data->num][0] = inserted_elements;
    thread_results[data->num][1] = found_elements;

    cout << "End of thread add_get n. " << data->num << endl;
    return NULL;
}

// each thread will delete a number of elements equal to the number
// it inserted in test_add_get()
void *test_del(void *params)
{
    th_params *data = (th_params *)params;
    cout << "Start of thread del n. " << data->num << endl;

    for (int i = 0; i < thread_results[data->num][0]; i++)
    {
        v_delete(data->vh, 0);
        free((char *)v_get(data->vh, 0));
    }

    cout << "End of thread del n. " << data->num << endl;
    return NULL;
}

TEST(Vector, VectorMultithread)
{
    string uaFile = CommandLineParamsParser::GetInstance()->GetValue("ua-file");
    if (uaFile == "")
    {
        uaFile = "/opt/ua_repo/ua-bench-real-traffic-data-small.txt";
    }

    // get the file's line number
    // neded to perform th mid-term checks
    int file_line_nbr = 0;
    ifstream uaStream(uaFile, ios::binary);

    if(!uaStream)
    {
        FAIL() << "Cannot find " << uaFile << " UA file" << endl;
    }

    string line;

    while (getline(uaStream, line) /* && lineCount < 100 */ )
    {
        //       line = trim(line);
        if(line.empty())
            continue;
        file_line_nbr++;
    }


    for (int numThreads = 1; numThreads <= 8; numThreads++)
    {
        v_h *vh = NULL;

        vh = v_create(vh, 10);
        ASSERT_NE((v_h *)0, vh);

        int size = v_len(vh);
        EXPECT_EQ(0, size);

        th_params parameters[numThreads];
        pthread_t pid[numThreads];
        int pthread_ret = 0;

        // perform the add_get activity
        for (int i = 0; i < numThreads; i++)
        {
            // clean results array
            thread_results[i][0] = 0;
            thread_results[i][1] = 0;

            parameters[i].vh = vh;
            parameters[i].num = i;
            parameters[i].uaFile = uaFile;
            pthread_ret = pthread_create(&pid[i], NULL, &test_add_get, (void *)&parameters[i]);
            ASSERT_EQ(0, pthread_ret);
        }

        for (int i = 0; i < numThreads; i++)
        {
            void *th_ret;
            pthread_join(pid[i], &th_ret);
        }

        // check mid-term results
        // vector length
        int size_after_add = v_len(vh);
        EXPECT_EQ(file_line_nbr * numThreads, size_after_add);

        // threads results
        // single thread inserted elements shold be equal to single thread found elements
        for (int i = 0; i < numThreads; i++)
        {
            EXPECT_EQ(thread_results[i][0], thread_results[i][1]);
        }

        // perform the del activity
        cout << "Start del"<< endl;

        for (int t = 0; t < numThreads; t++)
        {
            for (int i = 0; i < thread_results[t][0]; i++)
            {
                // deleting the last element is faster than deleting first element
                int size = v_len(vh);
                free((char *)v_get(vh, size-1));
                v_delete(vh, size-1);
            }
        }

        cout << "End del" << endl;

        int size_after_del = v_len(vh);
        EXPECT_EQ(0, size_after_del);

        v_destroy(vh);
    }

}
