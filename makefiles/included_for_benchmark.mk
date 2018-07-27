# Input variables are :
# LIB = library we are testing
# EXE = test executable name
# SRC = list of source files the test is made off.
# LOCAL_CFLAGS = list of private g++ flags
# RUN_OPTIONS

.SUFFIXES: .do
OBJ = $(SRC:.c=.o)
DOBJ = $(SRC:.c=.do)

INCLUDE_DIRS = -I ../../include -I../src -I../../libutil/src
CFLAGS = -O3 -fPIC --std=c99 -Wall -Wextra -Wcomment -pthread $(INCLUDE_DIRS) $(LOCAL_CFLAGS)
CFLAGS_DEBUG = -O0 -g3 -fPIC --std=c99 -Wall -Wextra -Wcomment -pthread $(INCLUDE_DIRS) $(LOCAL_CFLAGS)

LIBS = -L../../lib -L../src -L../../libutil/src -l$(LIB) -lutil -lpthread
LIBS_DEBUG = -L../../lib -L../src -L../../libutil/src -l$(LIB)-debug -lutil -lpthread

# release productions

benchlinux: $(OBJ)
	(cd ../src ; make -f lib$(LIB).mk releaseclean releaselinux)
	(cd ../../libutil/src ; make -f libutil.mk releaseclean releaselinux)
	$(CC) -o $(EXE) $(OBJ) $(LIBS) $(LOCAL_LDFLAGS) -lpthread

benchmacosx: $(OBJ)
	(cd ../src ; make -f lib$(LIB).mk releaseclean releaselinux)
	$(CC) -o $(EXE) $(OBJ) $(LIBS) $(LOCAL_LDFLAGS) -lpthread

benchwindows: $(OBJ)
	(cd ../src ; make -f lib$(LIB).mk releaseclean releaselinux)
	$(CC) -o $(EXE) $(OBJ) $(LIBS) $(LOCAL_LDFLAGS) -lpthread

# for profiling
dbenchlinux: $(DOBJ)
	(cd ../src ; make SANITIZE= -f lib$(LIB).mk debugclean debuglinux)
	(cd ../../libutil/src ; make SANITIZE= -f libutil.mk debugclean debuglinux)
	$(CC) -o $(EXE) $(DOBJ) $(LIBS_DEBUG) $(LOCAL_LDFLAGS) -lpthread

dbenchclean: indent
		(cd ../src ; make -f lib$(LIB).mk debugclean)
		rm -rf $(DOBJ) $(EXE) *.gcda *.gcno gtest-all.o ../../../../*_test_results.xml

benchrun:
	-./$(EXE) $(RUN_OPTIONS)

indent:
			-hostname | grep -v teamcity && uncrustify -l C --no-backup -c ../../crustify-config $(SRC)

benchclean: indent
	(cd ../src ; make -f lib$(LIB).mk releaseclean)
	rm -rf $(OBJ) $(EXE) *.gcda *.gcno gtest-all.o ../../../../*_test_results.xml

# production for debug compile
.c.do:
			$(CC) $(CFLAGS_DEBUG) -c $*.c -o $*.do
