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
-g3 $(SANITIZE) -I.. -I ../../libutil/src $(LOCAL_CXXFLAGS)
CXXFLAGS_RELEASE = -std=c++11 -pthread -fpermissive -isystem ../../googletest/include -isystem ../../googletest \
-O3 -I.. -I ../../libutil/src $(LOCAL_CXXFLAGS)

# debug productions

testlinux: $(OBJ)
	(cd .. ; make -f lib$(LIB).mk debugclean ; make -f lib$(LIB).mk debuglinux)
	$(CXX) -isystem ../../googletest/include -I ../../googletest/ -pthread -c ../../googletest/src/gtest-all.cc
	$(CXX) -o $(EXE) $(SANITIZE) $(OBJ) gtest-all.o -L ../ -l$(LIB)-debug $(LOCAL_LDFLAGS) -lpthread

testmacosx: $(OBJ)
	(cd .. ; make -f lib$(LIB).mk debugclean ; make -f lib$(LIB).mk debuglinux)
	$(CXX) -isystem ../../googletest/include -I ../../googletest/ -pthread -c ../../googletest/src/gtest-all.cc
	$(CXX) -o $(EXE) $(SANITIZE) $(OBJ) gtest-all.o -L ../ -l$(LIB)-debug $(LOCAL_LDFLAGS) -lpthread

testwindows: $(OBJ)
	(cd .. ; make -f lib$(LIB).mk debugclean ; make -f lib$(LIB).mk debuglinux)
	$(CXX) -isystem ../../googletest/include -I ../../googletest/ -pthread -c ../../googletest/src/gtest-all.cc
	$(CXX) -o $(EXE) $(SANITIZE) $(OBJ) gtest-all.o -L .. -l$(LIB)-debug $(LOCAL_LDFLAGS) -lpthread

testclean: indent
		rm -rf $(OBJ) $(EXE) *.gcda *.gcno gtest-all.o ../../*_test_results.xml

testrun:
			-./$(EXE) --gtest_output=xml:../../lib$(LIB)_test_results.xml $(RUN_OPTIONS)


# release compiled test productions

rtestlinux: $(ROBJ)
		(cd .. ; make -f lib$(LIB).mk releaseclean ; make -f lib$(LIB).mk releaselinux)
		$(CXX) -isystem ../../googletest/include -I ../../googletest/ -pthread -c ../../googletest/src/gtest-all.cc
		$(CXX) -o r$(EXE) $(ROBJ) gtest-all.o -L .. -l$(LIB) $(LOCAL_LDFLAGS) -lpthread

rtestmacosx: $(ROBJ)
		(cd .. ; make -f lib$(LIB).mk releaseclean ; make -f lib$(LIB).mk releaselinux)
		$(CXX) -isystem ../../googletest/include -I ../../googletest/ -pthread -c ../../googletest/src/gtest-all.cc
		$(CXX) -o r$(EXE) $(ROBJ) gtest-all.o -L ../ -l$(LIB) $(LOCAL_LDFLAGS) -lpthread

rtestwindows: $(ROBJ)
		(cd ../ ; make -f lib$(LIB).mk releaseclean ; make -f lib$(LIB).mk releaselinux)
		$(CXX) -isystem ../../googletest/include -I ../../googletest/ -pthread -c ../../googletest/src/gtest-all.cc
		$(CXX) -o r$(EXE) $(ROBJ) gtest-all.o -L ../ -l$(LIB) $(LOCAL_LDFLAGS) -lpthread

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
	$(CXX) -o $(EXE) $(LOCAL_CXXFLAGS) $(SANITIZE) --coverage $(OBJ) gtest-all.o -L ../ -l$(LIB)-coverage $(LOCAL_LDFLAGS) -lpthread
	mkdir -p ../coverage
	-./$(EXE) --gtest_output=xml:../../lib$(LIB)_test_results.xml $(RUN_OPTIONS)
	lcov --capture --directory ../ --base-directory ../ --output-file ../coverage/lcov.info

# production for release test compile
.cpp.ro:
				$(CXX) $(CXXFLAGS_RELEASE) -c $*.cpp -o $*.ro
