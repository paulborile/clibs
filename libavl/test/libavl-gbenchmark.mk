# LIB = library we are testing
# EXE = name of test executable
# SRC = list of source files the library is made of.
# LOCAL_CXXFLAGS = list of private Compiler flags
# LOCAL_LDFLAGS = list of linker flags
# RUN_OPTIONS = run options
# Available productions :
#
# test{linux,macosx,windows} : compiles tests
# testrun : run the tests
# testclean
# coveragelinux : compiles lib with coverage, compiles tests, runs tests

LIB = avl
EXE = gbenchavl
SRC = GBenchAVL.cpp
LOCAL_CXXFLAGS = 
LOCAL_LDFLAGS = 
# if any special options has to be passed to test run
# RUN_OPTIONS = --ua-file /opt/ua_repo/ua-bench-real-traffic-data-small.txt --gtest_filter=-*CountBananas*
RUN_OPTIONS = --benchmark_counters_tabular=true

include	../../makefiles/included_for_gbenchmark.mk
