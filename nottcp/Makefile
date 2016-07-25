CXX=g++
DEBUG ?= -DDEBUG
CXXFLAGS= -g -Wall $(DEBUG) -std=c++11
LDFLAGS = -pthread
EXECS=message-test rcsapp-client rcsapp-server
BIN_DIR=bin

_EXECS=$(patsubst %,$(BIN_DIR)/%, $(EXECS))
_SRCS=$(wildcard src/*.cpp)
_FAKE_OBJECTS=build/rcsapp-client.o build/rcsapp-server.o build/ucp.o
_REAL_OBJECTS=$(patsubst src/%.cpp,build/%.o, $(_SRCS))

UCP_OBJS=build/ucp.o build/net_util.o
RCS_OBJS=build/rcs.o build/rcs_socket.o build/message.o

.PHONY: all clean test rcsapp

all: $(BIN_DIR) $(_EXECS)

# test:
# 	tests/run_tests.sh

clean:
	rm -rf $(BIN_DIR) build/*

rcsapp: bin/rcsapp-client bin/rcsapp-server
	cp $^ .

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(_REAL_OBJECTS): build/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) $^ -c -o $@

$(_FAKE_OBJECTS): build/%.o: %.c rcs.h
	$(CXX) $(CXXFLAGS) $< -c -o $@

rcs.h: src/rcs.h
	cp $^ .

build/%.d: src/%.cpp
	$(CXX) $(CXXFLAGS) -MM -MF $@ $<

-include $(_SRCS:src/%.cpp=build/%.d)

####################################################################
########################     EXECS     #############################
####################################################################

$(BIN_DIR)/message-test: build/message.o build/message_test.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@

$(BIN_DIR)/rcsapp-client: build/rcsapp-client.o $(UCP_OBJS) $(RCS_OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@

$(BIN_DIR)/rcsapp-server: build/rcsapp-server.o $(UCP_OBJS) $(RCS_OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@
