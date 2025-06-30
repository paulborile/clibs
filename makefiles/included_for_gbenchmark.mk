# Input variables are :
# LIB = library we are benchmakring
# EXE = benchmark executable name
# SRC = list of source files the benchmark is made off.
# LOCAL_CXXFLAGS = list of private g++ flags
# RUN_OPTIONS

.SUFFIXES: .o

OBJ = $(SRC:.cpp=.o)
ROBJ = $(SRC:.cpp=.ro)

LIBS_RELEASE = -L ..

INCLUDE_DIRS = -I ..

CXXFLAGS = -std=c++14 -DNDEBUG -isystem ../../googlebenchmark/include $(INCLUDE_DIRS) \
-pthread -I../src/ $(LOCAL_CXXFLAGS)

gbenchlinux: $(OBJ)
	(cd .. ; make -f lib$(LIB).mk releaseclean ; make -f lib$(LIB).mk releaselinux)
# need to link to pthread since linking to lpthread there are potential issues with ordering of command line parameters if you use that.
# https://github.com/google/benchmark/blob/main/docs/platform_specific_build_instructions.md
#	$(CXX) -std=c++14  -isystem ../../googlebenchmark/include -pthread -c ../../googlebenchmark/src/benchmark.cc
	$(CXX) $(CXXFLAGS) -c ../../googlebenchmark/src/*.cc
	$(CXX) -o $(EXE) $(OBJ) benchmark_api_internal.o benchmark_name.o benchmark.o benchmark_register.o benchmark_runner.o check.o colorprint.o \
	commandlineflags.o complexity.o console_reporter.o counter.o csv_reporter.o json_reporter.o perf_counters.o reporter.o statistics.o \
	string_util.o sysinfo.o timers.o  -L ../src -l$(LIB) $(CXXFLAGS) $(LOCAL_LDFLAGS) $(LIBS_RELEASE) -pthread 

gbenchclean: indent
	rm -rf $(OBJ) $(EXE) *.gcda *.gcno gtest-all.o *.o ../../*_test_results.xml

gbenchrun:
	-./$(EXE)  $(RUN_OPTIONS)

indent:
	-hostname | grep -v teamcity && uncrustify -l CPP --no-backup --replace -c ../../crustify-config $(SRC)

# production for benchmark compile
.cpp.o:
				$(CXX) $(CXXFLAGS) -c $*.cpp -o $*.o
