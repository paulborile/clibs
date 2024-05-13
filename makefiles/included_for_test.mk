# Input variables are :
# LIB = library we are testing
# EXE = test executable name
# SRC = list of source files the test is made off.
# LOCAL_CXXFLAGS = list of private g++ flags
# RUN_OPTIONS

SANITIZE = -fsanitize=address
.SUFFIXES: .ro

OBJ = $(SRC:.cpp=.o)
ROBJ = $(SRC:.cpp=.ro)

CXXFLAGS = -std=c++11 -pthread -fpermissive -isystem ../../googletest/include -isystem ../../googletest -DDEBUG \
-isystem ../../picobench/include/ -g3 $(SANITIZE) -I.. -I ../../libthp -I ../../libchannel -I ../../libtiming \
$(LOCAL_CXXFLAGS)
CXXFLAGS_RELEASE = -std=c++11 -pthread -fpermissive -isystem ../../googletest/include -isystem ../../googletest \
-isystem ../../picobench/include/ -O3 -I.. -I ../../libthp -I ../../libchannel -I ../../libtiming \
$(LOCAL_CXXFLAGS)
LDFLAGS = -pthread -L../ -L../../libthp -L ../../libchannel -L ../../libtiming -l$(LIB)-debug $(LOCAL_LDFLAGS) -lpthread -lthp -lch -ltiming

# debug productions

testlinux: $(OBJ)
	(cd .. ; make -f lib$(LIB).mk debugclean ; make -f lib$(LIB).mk debuglinux)
	-which infer && (cd .. ; make -f lib$(LIB).mk releaseclean; infer run -- make -f lib$(LIB).mk releaselinux)
	$(CXX) $(CXXFLAGS) -isystem ../../googletest/include -I ../../googletest/ -pthread -c ../../googletest/src/gtest-all.cc
	$(CXX) -g3 -o $(EXE) $(SANITIZE) $(OBJ) gtest-all.o $(LDFLAGS)

testmacosx: $(OBJ)
	(cd .. ; make -f lib$(LIB).mk debugclean ; make -f lib$(LIB).mk debuglinux)
	$(CXX) $(CXXFLAGS) -isystem ../../googletest/include -I ../../googletest/ -pthread -c ../../googletest/src/gtest-all.cc
	$(CXX) -g3 -o $(EXE) $(SANITIZE) $(OBJ) gtest-all.o $(LDFLAGS)

testwindows: $(OBJ)
	(cd .. ; make -f lib$(LIB).mk debugclean ; make -f lib$(LIB).mk debuglinux)
	$(CXX) $(CXXFLAGS) -isystem ../../googletest/include -I ../../googletest/ -pthread -c ../../googletest/src/gtest-all.cc
	$(CXX) -g3 -o $(EXE) $(SANITIZE) $(OBJ) gtest-all.o $(LDFLAGS)

testclean: indent
	rm -rf $(OBJ) $(EXE) *.gcda *.gcno gtest-all.o ../../*_test_results.xml

testrun:
	-./$(EXE) --gtest_output=xml:../../lib$(LIB)_test_results.xml $(RUN_OPTIONS)


# release compiled test productions

rtestlinux: $(ROBJ)
		(cd .. ; make -f lib$(LIB).mk releaseclean ; make -f lib$(LIB).mk releaselinux)
		$(CXX) $(CXXFLAGS) -isystem ../../googletest/include -I ../../googletest/ -pthread -c ../../googletest/src/gtest-all.cc
		$(CXX) -o r$(EXE) $(ROBJ) gtest-all.o $(LDFLAGS)

rtestmacosx: $(ROBJ)
		(cd .. ; make -f lib$(LIB).mk releaseclean ; make -f lib$(LIB).mk releaselinux)
		$(CXX) $(CXXFLAGS) -isystem ../../googletest/include -I ../../googletest/ -pthread -c ../../googletest/src/gtest-all.cc
		$(CXX) -o r$(EXE) $(ROBJ) gtest-all.o $(LDFLAGS)

rtestwindows: $(ROBJ)
		(cd ../ ; make -f lib$(LIB).mk releaseclean ; make -f lib$(LIB).mk releaselinux)
		$(CXX) $(CXXFLAGS) -isystem ../../googletest/include -I ../../googletest/ -pthread -c ../../googletest/src/gtest-all.cc
		$(CXX) -o r$(EXE) $(ROBJ) gtest-all.o $(LDFLAGS)

rtestclean: indent
				rm -rf $(ROBJ) r$(EXE) *.gcda *.gcno gtest-all.o ../../*_test_results.xml
rtestrun:
	-./r$(EXE) --gtest_output="xml:../../lib$(LIB)_test_results.xml" $(RUN_OPTIONS)

indent:
		-hostname | grep -v teamcity && uncrustify -l CPP --no-backup -c ../../crustify-config *.cpp

# coverage run
coveragelinux: $(OBJ)
	(cd ../ ; make -f lib$(LIB).mk coverageclean ; make -f lib$(LIB).mk coveragelinux)
	$(CXX) -isystem ../../googletest/include -I ../../googletest/ -pthread -c ../../googletest/src/gtest-all.cc
	$(CXX) -g3 -o $(EXE) $(LOCAL_CXXFLAGS) $(SANITIZE) --coverage $(OBJ) gtest-all.o -L ../ -l$(LIB)-coverage $(LOCAL_LDFLAGS) \
	-lpthread -lthp -lch -ltiming
	mkdir -p ../coverage
	-./$(EXE) --gtest_output=xml:../../lib$(LIB)_test_results.xml $(RUN_OPTIONS)
	lcov --capture --directory ../ --base-directory ../ --output-file ../coverage/lcov.info

# production for release test compile
.cpp.ro:
				$(CXX) $(CXXFLAGS_RELEASE) -c $*.cpp -o $*.ro
