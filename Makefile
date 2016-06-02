CXX=g++
CXXFLAGS= -g -Wall
EXECS= addpeer
BIN_DIR=bin
_EXECS=$(patsubst %,$(BIN_DIR)/%, $(EXECS))
_OBJECTS=$(patsubst src/%.cpp,build/%.o, $(wildcard src/*.cpp))

.PHONY: all clean

all: $(BIN_DIR) build $(_EXECS)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

build:
	mkdir -p build

$(_OBJECTS): build/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) $^ -c -o $@

clean:
	rm -rf *.d *.o $(BIN_DIR) build

####################################################################

$(BIN_DIR)/addpeer: build/daemon.o build/peer.o build/add_peer.o
	$(CXX) $(CXXFLAGS) $^ -o $@
