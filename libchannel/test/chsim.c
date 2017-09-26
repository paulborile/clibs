#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "ch.h"

#define OPTS        "v"
#define BUF_SIZE    256

static void *Thread_Routine_Reader(void *);
static void *Thread_Routine_Writer(void *);


struct Data
{
    int pos;
    char name[50];
};
typedef struct Data Data;

int verbose = 0;


void get_command_line(int argc, char **argv)
{
    int c;

    extern char *optarg;

    while ((c = getopt(argc, argv, OPTS)) != EOF)
    {
        switch (c)
        {
        case 'v':
            verbose = 1;
            break;

        default:
            printf("Usage   :\n");
            printf("-v      : verbose\n");
            exit(1);
        }
    }
}

int main(int argc, char **argv)
{
    int i = 0;
    int ret = 0;
    int ciclo = 1;
    char *finger = NULL;
    char buffer[BUF_SIZE];
    void *p = NULL;
    char path[BUF_SIZE];
    void *pch_h = NULL;
    char attr[3];
    char val[3];
    int int_attr;
    int int_val;
    Data *object = NULL;
    Data object_result;
    char name[BUF_SIZE];
    char threads[3];
    int num_threads;
    pthread_t *thread_reader;
    pthread_t *thread_writer;
    pthread_attr_t tattr;

    pthread_attr_init(&tattr);
    pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&tattr, 50 * 1024);
    pthread_attr_setscope(&tattr, PTHREAD_SCOPE_SYSTEM);

    get_command_line(argc, argv);

    while (ciclo == 1)
    {
        ret = 0;
        finger = NULL;
        buffer[0] = '\0';


        if (verbose)
        {
            printf("ch-> ");
            fflush(stdout);
        }
        fgets(buffer, BUF_SIZE, stdin);

        if ((finger = (char *) strchr(buffer, '\n')) != NULL)
        {
            *finger = '\0';
        }

        if (strcmp(buffer, "help") == 0)
        {
            if (verbose)
            {
                printf("\nAvailable commands    :\n\n");
                printf("clean                   : empties the channel object\n");
                printf("destroy                 : destroys the channel object\n");
                printf("exit                    : exits from the program\n");
                printf("get                     : gets the first element inserted in the channel object\n");
                printf("getattr                 : gets a attribute value from the channel object\n");
                printf("help                    : this\n");
                printf("create                    : initializes the channel object\n");
                printf("peek                    : copies the first element inserted in the channel object\n");
                printf("put                     : puts a element in tail of the channel object\n");
                printf("puthead                 : puts a element in top of the channel object\n");
                printf("setattr                 : sets a attribute value of the channel object\n");
                printf("thread_reader           : creates threads that execute get\n");
                printf("thread_writer           : creates threads that execute put\n");
                printf("verbose                 : shows messages\n");
                printf("verbose_off             : does not show messages\n\n");
            }
        }
        else if (strcmp(buffer, "exit") == 0)
        {
            ciclo = 0;
        }
        else if (strcmp(buffer, "create") == 0)
        {
            if (p != NULL)
            {
                if (verbose)
                {
                    printf
                        ("destroy first !\n\n");
                }

                continue;
            }

            p = ch_create(NULL, sizeof(Data));

            if (p == NULL)
            {
                if (verbose)
                {
                    printf("ch_create failer.\n");
                }
            }
            else
            {
                pch_h = (ch_h *) p;

                if (verbose)
                {
                    printf("\n-----\n\n");
                    printf("magic values %x\n",
                           ((ch_h *) p)->magic);
                    printf("the elements number is %d\n\n",
                           ((ch_h *) p)->count);
                    printf("\n");
                }
            }
        }
        else if (strcmp(buffer, "setattr") == 0)
        {
            if (verbose)
            {
                printf("\nattribute or ESC to exit :\n");
                printf("\n100: CH_BLOCKING_MODE\n\n");
                printf("attr : ");
                fflush(stdout);
            }

            attr[0] = '\0';

            fgets(attr, 3, stdin);

            if ((finger = (char *) strchr(attr, '\n')) != NULL)
            {
                *finger = '\0';
            }

            if (strcmp(attr, "\033") == 0)
                continue;

            int_attr = atoi(attr);

            if (verbose)
            {
                printf("\nvalue or ESC to exit :\n");

                switch (int_attr)
                {
                case CH_BLOCKING_MODE:

                    printf("\n0: CH_ATTR_NON_BLOCKING_GET\n");
                    printf("\n1: CH_ATTR_BLOCKING_GET\n\n");

                    break;

                default:

                    printf("\n");
                }
                printf("value : ");
                fflush(stdout);
            }

            val[0] = '\0';

            fgets(val, 3, stdin);

            if ((finger = (char *) strchr(val, '\n')) != NULL)
            {
                *finger = '\0';
            }

            if (strcmp(val, "\033") == 0)
                continue;

            int_val = atoi(val);

            if ((ret = ch_setattr(p, int_attr, int_val)) != 0)
            {
                if (verbose)
                {
                    printf("ch_setattr FAILED. ret = %d\n\n", ret);
                }
            }
            else
            {
                if (verbose)
                {
                    printf("ch_setattr SUCCESS.\n\n");
                }
            }
        }
        else if (strcmp(buffer, "getattr") == 0)
        {
            if (verbose)
            {
                printf("\nattribute or ESC to exit :\n");
                printf("\n100: CH_BLOCKING_MODE\n");
                printf("\n200: CH_COUNT\n\n");
                printf("attr : ");
                fflush(stdout);
            }

            attr[0] = '\0';

            fgets(attr, 3, stdin);

            if ((finger = (char *) strchr(attr, '\n')) != NULL)
            {
                *finger = '\0';
            }

            if (strcmp(attr, "\033") == 0)
                continue;

            int_attr = atoi(attr);

            if ((ret = ch_getattr(p, int_attr, &int_val)) != 0)
            {
                if (verbose)
                {
                    printf("ch_getattr FAILED. ret = %d\n\n", ret);
                }
            }
            else
            {
                if (verbose)
                {
                    printf("ch_getattr SUCCESS.\n");

                    printf("\nint_attribute %d int_value %d\n\n", int_attr, int_val);
                }
            }
        }
        else if (strcmp(buffer, "destroy") == 0)
        {
            if ((ret = ch_destroy(p)) != 0)
            {
                if (verbose)
                {
                    printf("ch_destroy FAILED. ret %d\n\n", ret);
                }
            }
            else
            {
                if (verbose)
                {
                    printf("ch_destroy SUCCESS.\n\n");
                }
            }

            p = NULL;
        }
        else if (strcmp(buffer, "clean") == 0)
        {
            if ((ret = ch_clean(p)) < 0)
            {
                if (verbose)
                {
                    printf("ch_clean FAILED. ret %d\n\n", ret);
                }
            }
            else
            {
                if (verbose)
                {
                    printf("ch_clean SUCCESS - cleaned %d elements.\n\n", ret);
                }
            }
        }
        else if (strcmp(buffer, "put") == 0
                 || strcmp(buffer, "puthead") == 0)
        {
            int pos;

            name[0] = '\0';

            if (verbose)
            {
                printf("\nname or ESC to exit : ");
                fflush(stdout);
            }

            fgets(name, BUF_SIZE, stdin);

            if ((finger = (char *) strchr(name, '\n')) != NULL)
            {
                *finger = '\0';
            }

            if (strcmp(name, "\033") == 0)
                continue;

            ch_getattr(p, CH_COUNT, &pos);

            object = (Data *) malloc(sizeof(Data));

            object->pos = pos + 1;

            strcpy(object->name, name);

            if (strcmp(buffer, "put") == 0)
            {
                if ((ret = ch_put(p, object)) <= 0)
                {
                    if (verbose)
                    {
                        printf("ch_put FAILED. ret %d\n\n", ret);
                    }
                }
                else
                {
                    if (verbose)
                    {
                        printf("ch_put SUCCESS.\n");
                        printf("\nchannel elements %d.\n\n", ret);
                    }
                }
            }

            if (strcmp(buffer, "puthead") == 0)
            {
                if ((ret = ch_put_head(p, object)) <= 0)
                {
                    if (verbose)
                    {
                        printf("ch_put_head FAILED. ret %d\n\n", ret);
                    }
                }
                else
                {
                    if (verbose)
                    {
                        printf("ch_put_head SUCCESS.\n");
                        printf("\nchannel elements %d.\n\n", ret);
                    }
                }
            }
        }
        else if (strcmp(buffer, "get") == 0 || strcmp(buffer, "peek") == 0)
        {
            if (strcmp(buffer, "get") == 0)
            {
                if (ch_get(p, &object_result) == NULL)
                {
                    if (verbose)
                    {
                        printf("ch_get FAILED");
                    }
                }
                else
                {
                    if (verbose)
                    {
                        printf("ch_get SUCCESS.\n");
                        printf("\nname \"%s\" pos. %d.\n\n",
                               object_result.name, object_result.pos);
                    }
                }
            }

            if (strcmp(buffer, "peek") == 0)
            {
                if ( ch_peek(p, &object_result) == NULL)
                {
                    if (verbose)
                    {
                        printf("ch_peek FAILED. \n");
                    }
                }
                else
                {
                    if (verbose)
                    {
                        printf("ch_peek SUCCESS.\n");
                        printf("\nname \"%s\" pos. %d.\n\n",
                               object_result.name, object_result.pos);
                    }
                }
            }
        }
        else if (strcmp(buffer, "thread_reader") == 0)
        {
            threads[0] = '\0';

            if (verbose)
            {
                printf("\nhow many threads or ESC to exit : ");
                fflush(stdout);
            }

            fgets(threads, BUF_SIZE, stdin);

            if ((finger = (char *) strchr(threads, '\n')) != NULL)
            {
                *finger = '\0';
            }

            if (strcmp(threads, "\033") == 0)
                continue;

            num_threads = atoi(threads);

            ch_setattr(p, CH_BLOCKING_MODE, CH_ATTR_BLOCKING_GET);

            thread_reader =
                (pthread_t *) calloc(num_threads, sizeof(pthread_t));

            /* Creo i thread */
            printf("Creating threads : ");
            fflush(stdout);

            for (i = 0; i < num_threads; i++)
            {
                if ((ret = pthread_create(&thread_reader[i], &tattr,
                                          (void *) Thread_Routine_Reader,
                                          (void *) p)) < 0)
                {
                    printf("TEST FAILED !!! : pthread_create ret %d\n\n", ret);

                    return (0);
                }

                printf(".");
                fflush(stdout);
            }
            printf("\n");
        }
        else if (strcmp(buffer, "thread_writer") == 0)
        {
            threads[0] = '\0';

            if (verbose)
            {
                printf("\nhow many threads or ESC to exit : ");
                fflush(stdout);
            }

            fgets(threads, BUF_SIZE, stdin);

            if ((finger = (char *) strchr(threads, '\n')) != NULL)
            {
                *finger = '\0';
            }

            if (strcmp(threads, "\033") == 0)
                continue;

            num_threads = atoi(threads);

            thread_writer =
                (pthread_t *) calloc(num_threads, sizeof(pthread_t));

            /* Creo i thread */
            printf("Creating threads : ");
            fflush(stdout);

            for (i = 0; i < num_threads; i++)
            {
                if ((ret = pthread_create(&thread_writer[i], &tattr,
                                          (void *) Thread_Routine_Writer,
                                          (void *) p)) < 0)
                {
                    printf("TEST FAILED !!! : pthread_create ret %d\n\n", ret);

                    return (0);
                }

                printf(".");
                fflush(stdout);
            }
            printf("\n");
        }
        else if (strcmp(buffer, "verbose") == 0)
        {
            verbose = 1;
        }
        else
        {
            printf("channelsim-> COMMAND NOT AVAILABLE ");
            printf("(TYPE \'help\' FOR LISTING AVAILABLE COMMANDS)\n");
        }
    }

    /* Ritorno OK */
    return (0);
}


static void *Thread_Routine_Reader(void *p)
{
    Data d;

    ch_get(p, &d);
}


static void *Thread_Routine_Writer(void *p)
{
    int i;
    Data d;

    d.pos = 0;
    strcpy(d.name, "marco");

    for (i = 0; i < 98; i++)
    {
        ch_put(p, &d);
    }
}
