CXX=g++
DEBUG=
CXXFLAGS= -g -Wall $(DEBUG)
EXECS=addpeer allkeys removepeer addcontent lookupcontent removecontent
BIN_DIR=.
_EXECS=$(patsubst %,$(BIN_DIR)/%, $(EXECS))
_OBJECTS=$(patsubst src/%.cpp,build/%.o, $(wildcard src/*.cpp))

.PHONY: all clean test

all: build $(_EXECS)

test:
	tests/run_tests.sh

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

build:
	mkdir -p build

$(_OBJECTS): build/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) $^ -c -o $@

clean:
	rm -rf *.d *.o $(EXECS) build

####################################################################
########################     EXECS     #############################
####################################################################

addpeer: build build/daemon.o build/peer.o build/add_peer.o build/util.o build/messages.o build/mybind.o
	$(CXX) $(CXXFLAGS) $^ -o $@

allkeys: build build/allkeys.o build/util.o
	$(CXX) $(CXXFLAGS) $^ -o $@

removepeer: build build/remove_peer.o build/util.o build/client.o build/messages.o build/mybind.o
	$(CXX) $(CXXFLAGS) $^ -o $@

addcontent: build build/add_content.o build/util.o build/client.o build/messages.o build/mybind.o
	$(CXX) $(CXXFLAGS) $^ -o $@

lookupcontent: build build/lookup_content.o build/util.o build/client.o build/messages.o build/mybind.o
	$(CXX) $(CXXFLAGS) $^ -o $@

removecontent: build build/remove_content.o build/util.o build/client.o build/messages.o build/mybind.o
	$(CXX) $(CXXFLAGS) $^ -o $@

