CXX= g++
CXXFLAGS= -c -Wall -std=c++11 -DNDEBUG
CODEFILES= ex4.tar CacheFS.cpp Makefile README Answers.pdf
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
	-rm -vf *.o *.a *.tar
