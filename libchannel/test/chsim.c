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

char *gpcPrgName;
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
    int iRet = 0;
    int ciclo = 1;
    char *pcfinger = NULL;
    char acbuffer[BUF_SIZE];
    void *p = NULL;
    char acPath[BUF_SIZE];
    void *pch_h = NULL;
    char acAttr[3];
    char acVal[3];
    int Attr;
    int Val;
    Data *object = NULL;
    Data object_result;
    char acName[BUF_SIZE];
    char acThreads[3];
    int iNumThreads;
    pthread_t *thread_reader;
    pthread_t *thread_writer;
    pthread_attr_t tattr;

    pthread_attr_init(&tattr);
    pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&tattr, 50 * 1024);
    pthread_attr_setscope(&tattr, PTHREAD_SCOPE_SYSTEM);

    gpcPrgName = argv[0];

    get_command_line(argc, argv);

    while (ciclo == 1)
    {
        iRet = 0;
        pcfinger = NULL;
        acbuffer[0] = '\0';


        if (verbose)
        {
            printf("ch-> ");
            fflush(stdout);
        }
        fgets(acbuffer, BUF_SIZE, stdin);

        if ((pcfinger = (char *) strchr(acbuffer, '\n')) != NULL)
        {
            *pcfinger = '\0';
        }

        if (strcmp(acbuffer, "help") == 0)
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
        else if (strcmp(acbuffer, "exit") == 0)
        {
            ciclo = 0;
        }
        else if (strcmp(acbuffer, "create") == 0)
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

            p = ch_create(sizeof(Data));

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
        else if (strcmp(acbuffer, "setattr") == 0)
        {
            if (verbose)
            {
                printf("\nattribute or ESC to exit :\n");
                printf("\n100: CH_BLOCKING_MODE\n\n");
                printf("attr : ");
                fflush(stdout);
            }

            acAttr[0] = '\0';

            fgets(acAttr, 3, stdin);

            if ((pcfinger = (char *) strchr(acAttr, '\n')) != NULL)
            {
                *pcfinger = '\0';
            }

            if (strcmp(acAttr, "\033") == 0)
                continue;

            Attr = atoi(acAttr);

            if (verbose)
            {
                printf("\nvalue or ESC to exit :\n");

                switch (Attr)
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

            acVal[0] = '\0';

            fgets(acVal, 3, stdin);

            if ((pcfinger = (char *) strchr(acVal, '\n')) != NULL)
            {
                *pcfinger = '\0';
            }

            if (strcmp(acVal, "\033") == 0)
                continue;

            Val = atoi(acVal);

            if ((iRet = ch_setattr(p, Attr, Val)) != 0)
            {
                if (verbose)
                {
                    printf("ch_setattr FAILED. iRet = %d\n\n", iRet);
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
        else if (strcmp(acbuffer, "getattr") == 0)
        {
            if (verbose)
            {
                printf("\nattribute or ESC to exit :\n");
                printf("\n100: CH_BLOCKING_MODE\n");
                printf("\n200: CH_COUNT\n\n");
                printf("attr : ");
                fflush(stdout);
            }

            acAttr[0] = '\0';

            fgets(acAttr, 3, stdin);

            if ((pcfinger = (char *) strchr(acAttr, '\n')) != NULL)
            {
                *pcfinger = '\0';
            }

            if (strcmp(acAttr, "\033") == 0)
                continue;

            Attr = atoi(acAttr);

            if ((iRet = ch_getattr(p, Attr, &Val)) != 0)
            {
                if (verbose)
                {
                    printf("ch_getattr FAILED. iRet = %d\n\n", iRet);
                }
            }
            else
            {
                if (verbose)
                {
                    printf("ch_getattr SUCCESS.\n");

                    printf("\nAttribute %d Value %d\n\n", Attr, Val);
                }
            }
        }
        else if (strcmp(acbuffer, "destroy") == 0)
        {
            if ((iRet = ch_destroy(p)) != 0)
            {
                if (verbose)
                {
                    printf("ch_destroy FAILED. iRet %d\n\n", iRet);
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
        else if (strcmp(acbuffer, "clean") == 0)
        {
            if ((iRet = ch_clean(p)) < 0)
            {
                if (verbose)
                {
                    printf("ch_clean FAILED. iRet %d\n\n", iRet);
                }
            }
            else
            {
                if (verbose)
                {
                    printf("ch_clean SUCCESS - cleaned %d elements.\n\n", iRet);
                }
            }
        }
        else if (strcmp(acbuffer, "put") == 0
                 || strcmp(acbuffer, "puthead") == 0)
        {
            int pos;

            acName[0] = '\0';

            if (verbose)
            {
                printf("\nname or ESC to exit : ");
                fflush(stdout);
            }

            fgets(acName, BUF_SIZE, stdin);

            if ((pcfinger = (char *) strchr(acName, '\n')) != NULL)
            {
                *pcfinger = '\0';
            }

            if (strcmp(acName, "\033") == 0)
                continue;

            ch_getattr(p, CH_COUNT, &pos);

            object = (Data *) malloc(sizeof(Data));

            object->pos = pos + 1;

            strcpy(object->name, acName);

            if (strcmp(acbuffer, "put") == 0)
            {
                if ((iRet = ch_put(p, object)) <= 0)
                {
                    if (verbose)
                    {
                        printf("ch_put FAILED. iRet %d\n\n", iRet);
                    }
                }
                else
                {
                    if (verbose)
                    {
                        printf("ch_put SUCCESS.\n");
                        printf("\nchannel elements %d.\n\n", iRet);
                    }
                }
            }

            if (strcmp(acbuffer, "puthead") == 0)
            {
                if ((iRet = ch_put_head(p, object)) <= 0)
                {
                    if (verbose)
                    {
                        printf("ch_put_head FAILED. iRet %d\n\n", iRet);
                    }
                }
                else
                {
                    if (verbose)
                    {
                        printf("ch_put_head SUCCESS.\n");
                        printf("\nchannel elements %d.\n\n", iRet);
                    }
                }
            }
        }
        else if (strcmp(acbuffer, "get") == 0 || strcmp(acbuffer, "peek") == 0)
        {
            if (strcmp(acbuffer, "get") == 0)
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

            if (strcmp(acbuffer, "peek") == 0)
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
        else if (strcmp(acbuffer, "thread_reader") == 0)
        {
            acThreads[0] = '\0';

            if (verbose)
            {
                printf("\nhow many threads or ESC to exit : ");
                fflush(stdout);
            }

            fgets(acThreads, BUF_SIZE, stdin);

            if ((pcfinger = (char *) strchr(acThreads, '\n')) != NULL)
            {
                *pcfinger = '\0';
            }

            if (strcmp(acThreads, "\033") == 0)
                continue;

            iNumThreads = atoi(acThreads);

            ch_setattr(p, CH_BLOCKING_MODE, CH_ATTR_BLOCKING_GET);

            thread_reader =
                (pthread_t *) calloc(iNumThreads, sizeof(pthread_t));

            /* Creo i thread */
            printf("Creating threads : ");
            fflush(stdout);

            for (i = 0; i < iNumThreads; i++)
            {
                if ((iRet = pthread_create(&thread_reader[i], &tattr,
                                           (void *) Thread_Routine_Reader,
                                           (void *) p)) < 0)
                {
                    printf("TEST FAILED !!! : pthread_create ret %d\n\n", iRet);

                    return (0);
                }

                printf(".");
                fflush(stdout);
            }
            printf("\n");
        }
        else if (strcmp(acbuffer, "thread_writer") == 0)
        {
            acThreads[0] = '\0';

            if (verbose)
            {
                printf("\nhow many threads or ESC to exit : ");
                fflush(stdout);
            }

            fgets(acThreads, BUF_SIZE, stdin);

            if ((pcfinger = (char *) strchr(acThreads, '\n')) != NULL)
            {
                *pcfinger = '\0';
            }

            if (strcmp(acThreads, "\033") == 0)
                continue;

            iNumThreads = atoi(acThreads);

            thread_writer =
                (pthread_t *) calloc(iNumThreads, sizeof(pthread_t));

            /* Creo i thread */
            printf("Creating threads : ");
            fflush(stdout);

            for (i = 0; i < iNumThreads; i++)
            {
                if ((iRet = pthread_create(&thread_writer[i], &tattr,
                                           (void *) Thread_Routine_Writer,
                                           (void *) p)) < 0)
                {
                    printf("TEST FAILED !!! : pthread_create ret %d\n\n", iRet);

                    return (0);
                }

                printf(".");
                fflush(stdout);
            }
            printf("\n");
        }
        else if (strcmp(acbuffer, "verbose") == 0)
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
