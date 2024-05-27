# libch - a golang inspired channel library in C

Connect producer / consumer threads with a thread safe channel

```
    ...
    ch_h *ch = NULL;
    pthread_t th_writer, th_reader;
    int pthread_ret = 0, retval;
    void *th_ret;

    ch_h *ch = ch_create(NULL, CH_DATALEN_STRING);

    // for fixed size channel (default infinite), ch_put will block on channel full

    retval = ch_setattr(ch, CH_FIXED_SIZE, 1000);

    // default for ch_get is BLOCKING_MODE i.e. ch_get will wait for a new
    // message to be available in the channel. Non blocking mode is available though
    // retval = ch_setattr(ch, CH_BLOCKING_MODE, CH_ATTR_NON_BLOCKING_GETPUT);

    pthread_ret = pthread_create(&th_reader, NULL, &thread_reader, (void *)ch);

    for (i = 0; i < MSGS_SENT; i++)
    {
        char text[20];

        sprintf(text, "text %d", i);
        retval = ch_put(ch, text);
    }
    ch_put(ch, CH_ENDOFTRANSMISSION);
    pthread_join(th_reader, &th_ret);
    retval = ch_destroy(ch);
    ...

static void *thread_reader(void *ch)
{
    char text[50];
    int eot = 0;
    while (eot != CH_GET_ENDOFTRANSMISSION)
    {
        eot = ch_get(ch, text);

        if (eot < 0 && eot != CH_GET_ENDOFTRANSMISSION)
        {
            // error
        }
    }
    return NULL;
}

```

```
make -f libch.mk releaseclean releaselinux debugclean debuglinux
cd test
make -f libch-test.mk testclean testlinux testrun

```

