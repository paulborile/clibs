# Input variables are :
# LIB = library name
# SRC = list of source files the library is made of.
# RELEASE_CFLAGS = list of private Compiler flags
# DEBUG_CFLAGS = list of private Compiler flags

.SUFFIXES: .gml .do .co .mo

SANITIZE = -fsanitize=address

OBJ = $(SRC:.c=.o)
DOBJ = $(SRC:.c=.do)
# macosx debug objs
MOBJ = $(SRC:.c=.mo)
COBJ = $(SRC:.c=.co)

CFLAGS = -DNDEBUG -O3 -fPIC --std=c99 -Wall -Wextra -Wcomment -pthread -I . $(RELEASE_CFLAGS)
CFLAGS_DEBUG = -DDEBUG -g3 $(SANITIZE) -O0 -fPIC --std=c99 -Wall -Wextra -Wcomment -pthread -I . $(DEBUG_CFLAGS)
CFLAGS_DEBUG_MACOSX = -DDEBUG -g3 $(SANITIZE) -O0 -fPIC --std=c99 -Wall -Wextra -Wcomment -pthread -I . $(DEBUG_CFLAGS)
CFLAGS_COVERAGE = -DDEBUG -g3 $(SANITIZE) -O0 --coverage -fPIC --std=c99 -Wall -Wextra -Wcomment -pthread -I . $(DEBUG_CFLAGS)

ARFLAGS = crs

# release productions

releaselinux: $(OBJ)
	$(AR) $(ARFLAGS) lib$(LIB).a $(OBJ)
releasemacosx: $(OBJ)
	$(AR) $(ARFLAGS) lib$(LIB).a $(OBJ)
	ranlib lib$(LIB).a
releasewindows: $(OBJ)
	$(AR) $(ARFLAGS) lib$(LIB).a $(OBJ)
releaseclean: indent
	rm -rf $(OBJ) lib$(LIB).a

# debug productions

debuglinux: $(DOBJ)
	$(AR) $(ARFLAGS) lib$(LIB)-debug.a $(DOBJ)
debugmacosx: $(MOBJ)
	$(AR) $(ARFLAGS) lib$(LIB)-debug.a $(MOBJ)
	ranlib lib$(LIB)-debug.a
debugwindows: $(DOBJ)
	$(AR) $(ARFLAGS) lib$(LIB)-debug.a $(DOBJ)
debugclean:
	rm -rf $(DOBJ) $(MOBJ) lib$(LIB)-debug.a

# coverage library productions

coveragelinux: $(COBJ)
	$(AR) $(ARFLAGS) lib$(LIB)-coverage.a $(COBJ)

coverageclean:
	rm -rf $(COBJ) lib$(LIB)-coverage.a coverage/* *.gcda *.gcno

indent:
	-hostname | grep -v teamcity && uncrustify -l C --no-backup -c ../crustify-config *.[ch]

# production for debug compile
.c.do:
			$(CC) $(CFLAGS_DEBUG) -c $*.c -o $*.do

# production for coverage compile
.c.co:
			$(CC) $(CFLAGS_COVERAGE) -c $*.c -o $*.co

# production for debug compile
.c.mo:
			$(CC) $(CFLAGS_DEBUG_MACOSX) -c $*.c -o $*.mo
