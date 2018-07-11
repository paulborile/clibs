# GTEST_DIR := /home/silvio/Projects/ultra-fast-api/c-api/lib/googletest
GTEST_DIR := .

libgtest.a:
	g++ -isystem ${GTEST_DIR}/include -I${GTEST_DIR} \
        -pthread -c ${GTEST_DIR}/src/gtest-all.cc
	ar -rv libgtest.a gtest-all.o
