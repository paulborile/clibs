# LIB = library name
# SRC = list of source files the library is made of.
# RELEASE_CFLAGS = list of private Compiler flags for release build
# DEBUG_CFLAGS = list of private Compiler flags for debug build
# Available productions :
#
# release{linux,macosx,windows} : makes release version of the library
# releaseclean
# debug{linux,macosx,windows} : make debug version of the library with sanitize enabled
# debugclean
# coveragelinux : make coverage versione of library (lib<suffiz>-coverage.a)
# coverageclean

# library suffix
LIB = lru
SRC = ll.c lru.c
# any personal cflag needed by the library code
RELEASE_CFLAGS = -I../libfh
DEBUG_CFLAGS = -I../libfh

include	../makefiles/included_for_lib.mk
