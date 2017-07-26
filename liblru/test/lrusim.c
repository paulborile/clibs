
#include    <stdio.h>
#include    <string.h>
#include <pthread.h>
#include  <stdlib.h>
#include    "lru.h"

#define     BUF_SIZE              256
#define     MAX_KEY               128


int main(int argc, char **argv)
{
    char acbuffer[BUF_SIZE];
    char acnumber[BUF_SIZE];
    char key[BUF_SIZE];
    char *payload;
    FILE *in = stdin;
    int lru_err = 0;
    int iciclo = 1;
    char *finger;


    lru_t *l;

    while (iciclo == 1)
    {
        /* Stampo il prompt */
        printf("lrusim-> ");
        fflush(stdout);

        /* Metto il comando nel buffer di appoggio */
        fgets(acbuffer,BUF_SIZE,in);

        if((finger = strchr(acbuffer, '\n')) != NULL)
        {
            *finger = '\0';
        }

        /* Controllo il tipo di comando digitato */
        if(strcmp(acbuffer,"help") == 0)
        {
            printf("\nAvailable commands:\n\n");
            printf("create : create lru\n");
            printf("check  : check a key for presence in lru\n");
            printf("add    : adds a key in lru\n");
            printf("list   : adds a key in lru\n");
            printf("clear   : adds a key in lru\n");
        }
        else if(strcmp(acbuffer,"exit") == 0)
        {
            iciclo = 0;
        }
        else if(strcmp(acbuffer, "create") == 0)
        {
            printf("size of lru : ");
            fflush(stdout);

            fgets(acnumber,10,in);

            if((finger = strchr(acnumber,'\n')) != NULL )
            {
                *finger = '\0';
            }

            if(strcmp(acnumber,"\033") == 0)
            {
                continue;
            }

            int size = atoi(acnumber);

            l = lru_create(size);

            printf("\n");
        }
        else if(strcmp(acbuffer, "check") == 0)
        {
            printf("key : ");
            fflush(stdout);

            fgets(key,10,in);
            if((finger = strchr(key,'\n')) != NULL )
            {
                *finger = '\0';
            }

            lru_err = lru_check(l, key, (void *) &payload);
            printf("lru_err = %d\n", lru_err);
        }
        else if(strcmp(acbuffer, "add") == 0)
        {
            printf("key : ");
            fflush(stdout);

            fgets(key,10,in);
            if((finger = strchr(key,'\n')) != NULL )
            {
                *finger = '\0';
            }


            lru_err = lru_add(l, key, "the_payload");
            printf("lru_err = %d\n", lru_err);

        }
        else if(strcmp(acbuffer, "list") == 0)
        {
            lru_print(l);
        }
        else if(strcmp(acbuffer, "clear") == 0)
        {
            lru_clear(l);
        }
        else
        {
            printf("unknown command, type help for a list\n");
        }
    }

    /* Ritorno OK */
    return(0);
}
