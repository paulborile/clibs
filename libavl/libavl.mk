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
LIB = avl
SRC = avl.c avl_internal.c
# any personal cflag needed by the library code
RELEASE_CFLAGS =
DEBUG_CFLAGS =

include	../makefiles/included_for_lib.mk
