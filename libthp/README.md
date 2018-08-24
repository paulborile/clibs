# libthp - simple thread pool

Simple thread pool library in C. Easy to use :

```
	...
	// create thread pool with 10 running threads
    thp_h *t = thp_create(NULL, 10, &err);

	// add some work to thread
	thp_add(t, user_function, (void *) user_function_param);

	// wait that all jobs terminate
	thp_wait(t);

    thp_destroy(t);

```

To compile (you will also need libchannel) :

```
make -f libthp.mk releaseclean releaselinux debugclean debuglinux
cd test
make -f libthp-test.mk testclean testlinux testrun

```

