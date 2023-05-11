# Input variables are :
# LIB = library we are testing
# EXE = test executable name
# SRC = list of source files the test is made off.
# LOCAL_CFLAGS = list of private g++ flags
# RUN_OPTIONS

.SUFFIXES: .do
OBJ = $(SRC:.c=.o)
DOBJ = $(SRC:.c=.do)

INCLUDE_DIRS = -I ../../include -I.. -I../../libtiming
CFLAGS = -O3 -fPIC --std=c99 -Wall -Wextra -Wcomment -pthread $(INCLUDE_DIRS) $(LOCAL_CFLAGS)
CFLAGS_DEBUG = -O0 -g3 -fPIC --std=c99 -Wall -Wextra -Wcomment -pthread $(INCLUDE_DIRS) $(LOCAL_CFLAGS)

LIBS = -L../../lib -L.. -L../../libtiming -l$(LIB) -ltiming -lpthread
LIBS_DEBUG = -L../../lib -L.. -L../../libtiming -l$(LIB)-debug -ltiming -lpthread

# release productions

benchlinux: $(OBJ)
	(cd .. ; make -f lib$(LIB).mk releaseclean releaselinux)
	(cd ../../libtiming ; make -f libtiming.mk releaseclean releaselinux)
	$(CC) -o $(EXE) $(OBJ) $(LIBS) $(LOCAL_LDFLAGS) -lpthread

benchmacosx: $(OBJ)
	(cd .. ; make -f lib$(LIB).mk releaseclean releaselinux)
	$(CC) -o $(EXE) $(OBJ) $(LIBS) $(LOCAL_LDFLAGS) -lpthread

benchwindows: $(OBJ)
	(cd .. ; make -f lib$(LIB).mk releaseclean releaselinux)
	$(CC) -o $(EXE) $(OBJ) $(LIBS) $(LOCAL_LDFLAGS) -lpthread

# for profiling
dbenchlinux: $(DOBJ)
	(cd .. ; make SANITIZE= -f lib$(LIB).mk debugclean debuglinux)
	(cd ../../libtiming ; make SANITIZE= -f libtiming.mk debugclean debuglinux)
	$(CC) -o $(EXE) $(DOBJ) $(LIBS_DEBUG) $(LOCAL_LDFLAGS) -lpthread

dbenchclean: indent
		(cd .. ; make -f lib$(LIB).mk debugclean)
		rm -rf $(DOBJ) $(EXE) *.gcda *.gcno gtest-all.o ../../../../*_test_results.xml

benchrun:
	-./$(EXE) $(RUN_OPTIONS)

indent:
	uncrustify -l C --no-backup -c ../../crustify-config $(SRC)

benchclean: indent
	(cd .. ; make -f lib$(LIB).mk releaseclean)
	rm -rf $(OBJ) $(EXE) *.gcda *.gcno gtest-all.o ../../../../*_test_results.xml

# production for debug compile
.c.do:
			$(CC) $(CFLAGS_DEBUG) -c $*.c -o $*.do
