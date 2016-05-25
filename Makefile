CXX=g++
CXXFLAGS= -g -Wall
EXECS=daemon-test
_EXECS=$(patsubst %,bin/%, $(EXECS))
_OBJECTS=$(patsubst src/%.cpp,build/%.o, $(wildcard src/*.cpp))

.PHONY: all clean

all: bin build $(_EXECS)

bin:
	mkdir -p bin

build:
	mkdir -p build

$(_OBJECTS): build/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) $^ -c -o $@

clean:
	rm -rf *.d *.o bin build

####################################################################

bin/daemon-test: build/daemon.o build/daemon_test.o build/pickip.o
	$(CXX) $(CXXFLAGS) $^ -o $@
