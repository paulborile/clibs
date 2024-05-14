#include <gtest/gtest.h>

#include "ch.h"

#include <pthread.h>
#include <string.h>

#include <thread>
#include <chrono>
using namespace std;


// Variables for thread problems during development - Keep them for eventual future investigations
pthread_mutex_t p_mutex;
pthread_cond_t condvar;
int sharedcounter = 0;

struct _tst_struct
{
    pthread_mutex_t p_mutex;
    pthread_cond_t condvar;
    int counter;
};
typedef struct _tst_struct tst_s;

//
typedef struct _datablock
{
    int counter;
    char text[10];
} datablock;


int myfree(void *obj)
{
    free(obj);

    return 1;
}

#define MSGS_SENT 1000000

static void *thread_reader(void *ch)
{
    char stringa[50];
    int eot = 0;

    //cout << "READER: start thread" << endl;

    while (eot != CH_GET_ENDOFTRANSMISSION)
    {
        eot = ch_get((ch_h *)ch, stringa);

        if (eot < 0 && eot != CH_GET_ENDOFTRANSMISSION)
        {
            cout << "Reader error!!! Retval " << eot << endl;
        }

        //cout << "READER: read string <" << stringa << ">" << endl;
    }

    //cout << "READER: end of thread" << endl;

    return NULL;
}


static void *thread_writer(void *ch)
{
    int i;
    int retval = 0;

    //cout << "WRITER: start thread" << endl;

    for (i = 0; i < MSGS_SENT; i++)
    {
        char stringa[20];

        sprintf(stringa, "Stringa %d", i);
        retval = ch_put((ch_h *)ch, stringa);

        if (retval < 0)
        {
            cout << "Writer error!!! Retval " << retval << endl;
        }

        // << "WRITER: wrote string <" << stringa << ">" << endl;
    }

    ch_put((ch_h *)ch, CH_ENDOFTRANSMISSION);

    //cout << "WRITER: end of thread" << endl;

    return NULL;
}

static void *thread_pointer_reader(void *ch)
{
    datablock *element = NULL;
    int eot = 0;

    //cout << "POINTER READER: start thread" << endl;

    while (eot != CH_GET_ENDOFTRANSMISSION)
    {
        eot = ch_get((ch_h *)ch, &element);

        if (eot != CH_GET_ENDOFTRANSMISSION)
        {
            if (eot < 0)
            {
                cout << "Pointer reader error!!! Retval " << eot << endl;
            }
            else if (element != NULL)
            {
                //cout << "POINTER READER: read element <" << element->counter << ">, text value <" << element->text << ">" << endl;

                free(element);
            }
        }
    }

    //cout << "POINTER READER: end of thread" << endl;

    return NULL;
}

// Pass a struct pointer to the channel: it will put a void * or a copy of struct data depending on data type defined in channel creation
static void *thread_pointer_writer(void *ch)
{
    int i;
    int retval = 0;
    datablock *element = NULL;

    //cout << "POINTER WRITER: start thread" << endl;

    for (i = 0; i < MSGS_SENT; i++)
    {
        element = (datablock *)malloc(sizeof(datablock));
        element->counter = i;
        sprintf(element->text, "Text %d", i);

        retval = ch_put((ch_h *)ch, element);

        if (retval < 0)
        {
            //cout << "Pointer writer error!!! Retval " << retval << endl;
        }

        //cout << "POINTER WRITER: wrote element <" << i << ">" << endl;

        if (((ch_h *)ch)->datalen != CH_DATALEN_VOIDP)
        {
            // Channel transports structs. Put make a copy of data, we must free allocated memory to avoid leak.
            free(element);
        }
    }

    ch_put((ch_h *)ch, CH_ENDOFTRANSMISSION);

    //cout << "POINTER WRITER: end of thread" << endl;

    return NULL;
}

// Error conditions of put method
TEST(CH, put_error_conditions)
{
    ch_h *ch = NULL;
    char block[20];
    block[0] = '\0';

    int retval = ch_put(ch, block);
    EXPECT_EQ(CH_WRONG_PARAM, retval);

    ch = (ch_h *)ch_create(NULL, 20);
    ASSERT_NE((ch_h *)0, ch);

    retval = CH_OK;
    retval = ch_put(ch, NULL);
    EXPECT_EQ(CH_WRONG_PARAM, retval);

    ch_destroy(ch);
}

// Error conditions of read methods (get (remove object from channel) and peek (read it without remove)
TEST(CH, read_error_conditions)
{
    ch_h *ch = NULL;
    char block[20];
    block[0] = '\0';

    int result = ch_get(ch, block);
    EXPECT_EQ(CH_WRONG_PARAM, result);

    result = CH_OK;
    result = ch_peek(ch, block);
    EXPECT_EQ(CH_WRONG_PARAM, result);

    ch = (ch_h *)ch_create(NULL, 20);
    ASSERT_NE((ch_h *)0, ch);

    result = CH_OK;
    result = ch_get(ch, NULL);
    EXPECT_EQ(CH_WRONG_PARAM, result);

    result = CH_OK;
    result = ch_peek(ch, NULL);
    EXPECT_EQ(CH_WRONG_PARAM, result);

    ch_destroy(ch);
}

// Error conditions of setattr/getattr methods
TEST(CH, attr_error_conditions)
{
    ch_h *ch = NULL;
    int value = 0;

    int retval = ch_setattr(ch, CH_COUNT, 0);
    EXPECT_EQ(CH_BAD_HANDLE, retval);

    retval = ch_getattr(ch, CH_COUNT, &value);
    EXPECT_EQ(CH_BAD_HANDLE, retval);

    ch = (ch_h *)ch_create(NULL, 20);
    ASSERT_NE((ch_h *)0, ch);

    retval = ch_setattr(ch, CH_COUNT, 0);
    EXPECT_EQ(CH_WRONG_ATTR, retval);

    retval = ch_setattr(ch, CH_BLOCKING_MODE, 3);
    EXPECT_EQ(CH_WRONG_VALUE, retval);

    retval = ch_getattr(ch, 500, &value);
    EXPECT_EQ(CH_WRONG_ATTR, retval);

//  No test on null pointer for attribute value returned.
//    retval = ch_getattr(ch, CH_COUNT, NULL);
//    EXPECT_EQ(CH_WRONG_PARAM, retval);

    ch_destroy(ch);
}

// Error conditions of create/clean/destroy methods
TEST(CH, general_error_conditions)
{
    ch_h *ch = NULL;

    int retval = ch_clean(ch, NULL);
    EXPECT_EQ(CH_BAD_HANDLE, retval);

    retval = ch_destroy(ch);
    EXPECT_EQ(CH_BAD_HANDLE, retval);

    ch = (ch_h *)ch_create(NULL, 20);
    ASSERT_NE((ch_h *)0, ch);

    // channel don't contains void pointer, clean won't accept a free function
    retval = ch_clean(ch, myfree);
    EXPECT_EQ(CH_FREE_NOT_REQUESTED, retval);

    retval = ch_clean(ch, NULL);
    EXPECT_EQ(CH_OK, retval);

    retval = ch_destroy(ch);
    EXPECT_EQ(CH_OK, retval);
}

// Simple test: one thread writes in a channel, the other read.
TEST(CH, simple_channel_test)
{
    ch_h *ch = NULL;
    pthread_t th_writer, th_reader;
    pthread_attr_t tattr;
    int pthread_ret = 0, retval;
    void *th_ret;

    ch = (ch_h *)ch_create(NULL, CH_DATALEN_STRING);
    ASSERT_NE((ch_h *)0, ch);

    // General settings for threads
    // pthread_attr_init(&tattr);
    // pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
    // pthread_attr_setscope(&tattr, PTHREAD_SCOPE_SYSTEM);

//    retval = ch_setattr(ch, CH_BLOCKING_MODE, CH_ATTR_NON_BLOCKING_GET);
//    EXPECT_EQ(CH_OK, retval);

    pthread_ret = pthread_create(&th_reader, NULL, &thread_reader, (void *)ch);
    ASSERT_EQ(0, pthread_ret);

//    sleep(5);

    pthread_ret = pthread_create(&th_writer, NULL, &thread_writer, (void *)ch);
    ASSERT_EQ(0, pthread_ret);

    pthread_join(th_writer, &th_ret);
    pthread_join(th_reader, &th_ret);

    retval = ch_destroy(ch);
    EXPECT_EQ(CH_OK, retval);
}

// Test on peek function
TEST(CH, peek_test)
{
    ch_h *ch = NULL;
    int counter = 0, retval;
    char element[50], oldelement[50];

    ch = (ch_h *)ch_create(NULL, CH_DATALEN_STRING);
    ASSERT_NE((ch_h *)0, ch);

    // Fill the queue
    for (int ind = 0; ind < 10; ind++)
    {
        char stringa[20];
        sprintf(stringa, "Stringa %d", ind);
        counter = ch_put(ch, stringa);
        EXPECT_EQ(ind + 1, counter);
    }

    // Read first element
    retval = CH_OK;
    retval = ch_peek(ch, element);
    EXPECT_EQ(CH_OK, retval);

    // Save element value
    strcpy(oldelement, element);

    // Read again first element
    retval = CH_OK;
    retval = ch_peek(ch, element);
    EXPECT_EQ(CH_OK, retval);

    // It must be the same
    EXPECT_STREQ(oldelement, element);

    ch_put_head(ch, (void *)"The first one!");

    // Read again first element
    retval = CH_OK;
    retval = ch_peek(ch, element);
    EXPECT_EQ(CH_OK, retval);

    // It must be different now
    EXPECT_STRNE(oldelement, element);

    // Clean the queue
    ch_clean(ch, NULL);

    // Now read the queue must return no data
    retval = CH_OK;
    retval = ch_peek(ch, element);
    EXPECT_EQ(CH_GET_NODATA, retval);

    retval = ch_destroy(ch);
    EXPECT_EQ(CH_OK, retval);
}

// Test on get/set attr function
TEST(CH, attr_test)
{
    ch_h *ch = NULL;
    int limit = 30;
    int value, retval, counter;
    char element[50];

    ch = (ch_h *)ch_create(NULL, CH_DATALEN_STRING);
    ASSERT_NE((ch_h *)0, ch);

    // Fill the queue
    for (int ind = 0; ind < limit; ind++)
    {
        char stringa[20];
        sprintf(stringa, "Stringa %d", ind);
        counter = ch_put(ch, stringa);
        EXPECT_EQ(ind + 1, counter);
    }

    // Number of elements must be equal to limit
    retval = CH_OK;
    retval = ch_getattr(ch, CH_COUNT, &value);
    EXPECT_EQ(CH_OK, retval);
    EXPECT_EQ(limit, value);

    // Queue actually set as blocking
    retval = CH_OK;
    retval = ch_getattr(ch, CH_BLOCKING_MODE, &value);
    EXPECT_EQ(CH_OK, retval);
    EXPECT_EQ(CH_ATTR_BLOCKING_GETPUT, value);

    // Set queue as non blocking and check it
    retval = CH_OK;
    retval = ch_setattr(ch, CH_BLOCKING_MODE, CH_ATTR_NON_BLOCKING_GETPUT);
    EXPECT_EQ(CH_OK, retval);
    retval = CH_OK;
    retval = ch_getattr(ch, CH_BLOCKING_MODE, &value);
    EXPECT_EQ(CH_OK, retval);
    EXPECT_EQ(CH_ATTR_NON_BLOCKING_GETPUT, value);

    // Read first element
    retval = CH_OK;
    retval = ch_get(ch, element);
    EXPECT_EQ(CH_OK, retval);

    // Now queue contains limit - 1 elements
    retval = CH_OK;
    retval = ch_getattr(ch, CH_COUNT, &value);
    EXPECT_EQ(CH_OK, retval);
    EXPECT_EQ(limit - 1, value);

    // Clean the queue
    ch_clean(ch, NULL);

    // Now queue contains no elements
    retval = CH_OK;
    retval = ch_getattr(ch, CH_COUNT, &value);
    EXPECT_EQ(CH_OK, retval);
    EXPECT_EQ(0, value);

    retval = ch_destroy(ch);
    EXPECT_EQ(CH_OK, retval);
}

// Void pointer test: one thread writes in a channel objects (pointer to a specific struct type), the other read and check data.
TEST(CH, pointer_channel_test)
{
    ch_h *ch = NULL;
    pthread_t th_writer, th_reader;
    pthread_attr_t tattr;
    int pthread_ret = 0, retval;
    void *th_ret;

    ch = (ch_h *)ch_create(NULL, CH_DATALEN_VOIDP);
    ASSERT_NE((ch_h *)0, ch);

    // General settings for threads
    //pthread_attr_init(&tattr);
    //pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
    //pthread_attr_setscope(&tattr, PTHREAD_SCOPE_SYSTEM);

//    retval = ch_setattr(ch, CH_BLOCKING_MODE, CH_ATTR_NON_BLOCKING_GET);
//    EXPECT_EQ(CH_OK, retval);

    pthread_ret = pthread_create(&th_reader, NULL, &thread_pointer_reader, (void *)ch);
    ASSERT_EQ(0, pthread_ret);

//    sleep(5);

    pthread_ret = pthread_create(&th_writer, NULL, &thread_pointer_writer, (void *)ch);
    ASSERT_EQ(0, pthread_ret);

    pthread_join(th_writer, &th_ret);
    pthread_join(th_reader, &th_ret);

    retval = ch_destroy(ch);
    EXPECT_EQ(CH_OK, retval);
}

// Check that clean method works without problems with every kind of data in channel
TEST(CH, clean_execution_test)
{
    ch_h *ch = NULL;
    pthread_t th_writer;
    pthread_attr_t tattr;
    int pthread_ret = 0, retval = CH_OK;
    void *th_ret;
    datablock *element = NULL;
    datablock elem;
    char stringa[50];

    // First test: using void pointer as data
    ch = (ch_h *)ch_create(NULL, CH_DATALEN_VOIDP);
    ASSERT_NE((ch_h *)0, ch);

    // General settings for threads
    //pthread_attr_init(&tattr);
    //pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
    //pthread_attr_setscope(&tattr, PTHREAD_SCOPE_SYSTEM);

    pthread_ret = pthread_create(&th_writer, NULL, &thread_pointer_writer, (void *)ch);
    ASSERT_EQ(0, pthread_ret);

    pthread_join(th_writer, &th_ret);

    // Clean channel
    retval = ch_clean(ch, myfree);
    EXPECT_EQ(CH_OK, retval);

    // Check if channel is empty
    retval = CH_OK;
    retval = ch_peek((ch_h *)ch, &element);
    EXPECT_EQ(CH_GET_NODATA, retval) << "element block value read " << element->text;

    retval = CH_OK;
    retval = ch_destroy(ch);
    EXPECT_EQ(CH_OK, retval);

    // Second test: using strings as data
    ch = (ch_h *)ch_create(NULL, CH_DATALEN_STRING);
    ASSERT_NE((ch_h *)0, ch);

    // General settings for threads
    pthread_attr_init(&tattr);
    pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setscope(&tattr, PTHREAD_SCOPE_SYSTEM);

    pthread_ret = pthread_create(&th_writer, NULL, &thread_writer, (void *)ch);
    ASSERT_EQ(0, pthread_ret);

    pthread_join(th_writer, &th_ret);

    // Clean channel
    retval = CH_OK;
    retval = ch_clean(ch, NULL);
    EXPECT_EQ(CH_OK, retval);

    // Check if channel is empty
    retval = CH_OK;
    retval = ch_peek((ch_h *)ch, stringa);
    EXPECT_EQ(CH_GET_NODATA, retval) << "element block value read " << element->text;

    retval = CH_OK;
    retval = ch_destroy(ch);
    EXPECT_EQ(CH_OK, retval);

    // Third test: using struct as data (data copied in transport element)
    ch = (ch_h *)ch_create(NULL, sizeof(datablock));
    ASSERT_NE((ch_h *)0, ch);

    // General settings for threads
    pthread_attr_init(&tattr);
    pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setscope(&tattr, PTHREAD_SCOPE_SYSTEM);

    pthread_ret = pthread_create(&th_writer, NULL, &thread_pointer_writer, (void *)ch);
    ASSERT_EQ(0, pthread_ret);

    pthread_join(th_writer, &th_ret);

    // Clean channel
    retval = CH_OK;
    retval = ch_clean(ch, NULL);
    EXPECT_EQ(CH_OK, retval);

    // Check if channel is empty
    retval = CH_OK;
    retval = ch_peek((ch_h *)ch, &elem);
    EXPECT_EQ(CH_GET_NODATA, retval) << "element block value read " << element->text;

    retval = CH_OK;
    retval = ch_destroy(ch);
    EXPECT_EQ(CH_OK, retval);
}

// Check if channel will be cleaned when end of transmission isn't last element in channel
TEST(CH, eot_in_the_middle_test)
{
    ch_h *ch = NULL;
    int retval = CH_OK;
    datablock *element = NULL;
    datablock *element2 = NULL;

    ch = (ch_h *)ch_create(NULL, CH_DATALEN_VOIDP);
    ASSERT_NE((ch_h *)0, ch);

    // Add a valid element
    element = (datablock *)malloc(sizeof(datablock));
    element->counter = 1;
    sprintf(element->text, "AAAAAAAA");

    retval = ch_put((ch_h *)ch, element);

    // Add end of transmission
    retval = CH_OK;
    retval = ch_put((ch_h *)ch, CH_ENDOFTRANSMISSION);

    // Add another element after end of transmission
    element2 = (datablock *)malloc(sizeof(datablock));
    element2->counter = 1;
    sprintf(element2->text, "BBBBBBBBB");

    retval = CH_OK;
    retval = ch_put((ch_h *)ch, element2);

    // Clean channel
    retval = CH_OK;
    retval = ch_clean(ch, NULL);
    EXPECT_EQ(CH_OK, retval);

    // Check if channel is empty
    retval = CH_OK;
    retval = ch_peek((ch_h *)ch, &element);
    EXPECT_EQ(CH_GET_NODATA, retval) << "element block value read " << element->text;

    retval = CH_OK;
    retval = ch_destroy(ch);
    EXPECT_EQ(CH_OK, retval);

    // Free data
    free(element);
    free(element2);
}

struct th_data
{
    ch_h *ch;
    int thread_number;
    int num_reader;
    int num_writer;
    int num_messages;
};

int msgs_per_thread[16]; // don't use more that 16 threads

void *writer(void *v)
{
    struct th_data *t = (struct th_data *)v;
    const char *str = "a string address to use";
    // send data
    for (int i=0; i<t->num_messages/t->num_writer; i++)
    {
        ch_put(t->ch, (void *) str);
    }

    // terminate sending 1 EOT for each thread. reader thread will recevie EOT and terminate
    // only writer thread 0 does this and after a while to be sure that if there are other writers
    // all have finished
    if ( t->thread_number == 0 )
    {
        this_thread::sleep_for(chrono::seconds(3)); // coarse way of letting all other writers thread terminate
        for (int i =0; i<t->num_reader; i++)
        {
            ch_put(t->ch, CH_ENDOFTRANSMISSION);
        }
    }
    return NULL;
}

void *reader(void *v)
{
    struct th_data *t = (struct th_data *)v;

    char *d;
    int rc;
    while (( rc = ch_get(t->ch, &d)) != CH_GET_ENDOFTRANSMISSION )
    {
        if (rc == CH_OK)
        {
            msgs_per_thread[t->thread_number]++;
        }
        else
        {
            // this condition should never happen : in blocking mode ch_get() should
            // always return CH_OK or CH_GET_ENDOFTRANSMISSION. We should loop in cond_wait()
            printf("reader %d, error %d in ch_get\n", t->thread_number, rc);
        }
    }

    return NULL;
}

TEST(CH, MT_N_Writer_M_Reader)
{
    ch_h *ch = NULL;
    pthread_t th_writer[16], th_reader[16];
    int pthread_ret = 0, retval;
    void *th_ret;

    ch = (ch_h *)ch_create(NULL, CH_DATALEN_VOIDP);
    ASSERT_NE((ch_h *)0, ch);

#define NUM_READER 8
#define NUM_WRITER 2
#define NUM_MESSAGES (1000000 * NUM_READER)

    struct th_data rtdata[16];
    struct th_data wtdata[16];

    // start writers
    for (int i = 0; i<NUM_WRITER; i++)
    {
        memset(&wtdata[i], 0, sizeof(struct th_data));

        wtdata[i].num_reader = NUM_READER;
        wtdata[i].num_writer = NUM_WRITER;
        wtdata[i].num_messages = NUM_MESSAGES;
        wtdata[i].ch = ch;
        wtdata[i].thread_number = i;
        pthread_ret = pthread_create(&th_writer[i], NULL, &writer, (void *)&wtdata[i]);
        ASSERT_EQ(0, pthread_ret);
    }

    printf("channels waiting threads %d\n", ch->get_waiting_threads);

    // start readers
    for (int i = 0; i<NUM_READER; i++)
    {
        memset(&rtdata[i], 0, sizeof(struct th_data));

        rtdata[i].num_reader = NUM_READER;
        rtdata[i].num_writer = NUM_WRITER;
        rtdata[i].num_messages = NUM_MESSAGES;
        rtdata[i].ch = ch;
        rtdata[i].thread_number = i;
        pthread_ret = pthread_create(&th_reader[i], NULL, &reader, (void *)&rtdata[i]);
        ASSERT_EQ(0, pthread_ret);
    }

    printf("channels waiting threads %d\n", ch->get_waiting_threads);

    for (int i = 0; i<NUM_WRITER; i++)
    {
        pthread_join(th_writer[i], &th_ret);
    }

    for (int i = 0; i<NUM_READER; i++)
    {
        pthread_join(th_reader[i], &th_ret);
    }

    printf("channels waiting threads %d\n", ch->get_waiting_threads);

    int sum = 0;
    for (int i = 0; i<NUM_READER; i++)
    {
        printf("reader %d : messages processed %d\n", i, msgs_per_thread[i]);
        sum += msgs_per_thread[i];
    }

    EXPECT_EQ(NUM_MESSAGES, sum);

    int channel_count_at_end;
    EXPECT_EQ(CH_OK, ch_getattr(ch, CH_COUNT, &channel_count_at_end));
    EXPECT_EQ(0, channel_count_at_end);

    retval = ch_destroy(ch);
    EXPECT_EQ(CH_OK, retval);
}
