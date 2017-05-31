CXX= g++
CXXFLAGS= -c -Wall -std=c++11 -DNDEBUG
CODEFILES= ex4.tar CacheFS.cpp Makefile README
LIBOBJECTS= CacheFS.o


# Default
default: CacheFS


# Executables
CacheFS: CacheFS.o
	ar rcs CacheFS.a $(LIBOBJECTS)
	-rm -f *.o


# Object Files
CacheFS.o: CacheFS.h CacheFS.cpp
	$(CXX) $(CXXFLAGS) CacheFS.cpp -o CacheFS.o


# tar
tar:
	tar -cvf $(CODEFILES)


# Other Targets
clean:
	-rm -vf *.o *.a *.tar Search


# Valgrind
Valgrind: CacheFS MyTest.cpp
	$(CXX) -g -Wall -std=c++11 MyTest.cpp -L. CacheFS.a -o Valgrind
	valgrind --leak-check=full --show-possibly-lost=yes --show-reachable=yes --undef-value-errors=yes ./Valgrind
	-rm -vf *.o *.a Valgrind


# MyTest
MyTest: CacheFS MyTest.cpp
	$(CXX) $(CXXFLAGS) MyTest.cpp -o MyTest.o
	$(CXX) MyTest.o -L. CacheFS.a -o MyTest
	./MyTest
	-rm -vf *.o *.a MyTest

