# LIB = library we are benchmarking
# EXE = name of bench executable
# SRC = list of source files the benchmark is made of.
# LOCAL_CFLAGS = list of private Compiler flags
# LOCAL_LDFLAGS = list of private linker flags
# RUN_OPTIONS = bench run options
# Available productions :
#
# bench{linux,macosx,windows} : compiles benchmarks
# benchrun : run the benchmarks
# benchclean

LIB = uat
EXE = uattiming
SRC = uattiming.c
LOCAL_CFLAGS =
LOCAL_LDFLAGS =
# if any special options has to be passed to benchrun
RUN_OPTIONS =

include	../../makefiles/included_for_benchmark.mk
