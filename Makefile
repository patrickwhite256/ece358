CXX=g++
CXXFLAGS= -g -Wall
EXECS=peer-test
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

bin/peer-test: build/peer.o build/peer_test.o build/pickip.o
	$(CXX) $(CXXFLAGS) $^ -o $@
