CXX=g++
DEBUG ?= -DDEBUG
CXXFLAGS= -g -Wall $(DEBUG)
EXECS=addpeer allkeys removepeer addcontent
BIN_DIR=bin
_EXECS=$(patsubst %,$(BIN_DIR)/%, $(EXECS))
_OBJECTS=$(patsubst src/%.cpp,build/%.o, $(wildcard src/*.cpp))

.PHONY: all clean test

all: $(BIN_DIR) build $(_EXECS)

test:
	tests/run_tests.sh

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

build:
	mkdir -p build

$(_OBJECTS): build/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) $^ -c -o $@

clean:
	rm -rf *.d *.o $(BIN_DIR) build

####################################################################
########################     EXECS     #############################
####################################################################

$(BIN_DIR)/addpeer: build/daemon.o build/peer.o build/add_peer.o build/util.o build/messages.o
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BIN_DIR)/allkeys: build/allkeys.o build/util.o
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BIN_DIR)/removepeer: build/remove_peer.o build/util.o build/client.o build/messages.o
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BIN_DIR)/addcontent: build/add_content.o build/util.o build/client.o build/messages.o
	$(CXX) $(CXXFLAGS) $^ -o $@
