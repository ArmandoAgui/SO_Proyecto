CC ?= cc
CPPFLAGS += -Iinclude
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -g

TARGET := build/dsm-node
SOURCES := src/main.c $(wildcard src/*/*.c)
OBJECTS := $(patsubst src/%.c,build/%.o,$(SOURCES))
DEPS := $(OBJECTS:.o=.d)

.DEFAULT_GOAL := all
.PHONY: all run test clean help

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(LDLIBS) -o $@

build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

run: $(TARGET)
	./$(TARGET) $(ARGS)

build/network-test: tests/unit/network_test.c src/network/network.c include/dsm/network.h include/dsm/types.h
	@mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) tests/unit/network_test.c src/network/network.c $(LDLIBS) -o $@

test: $(TARGET) build/network-test
	./build/network-test
	python3 tests/integration/test_sockets.py

clean:
	rm -rf build

help:
	@echo "make       Compile the Phase 1 TCP nodes"
	@echo "make run ARGS='--id 0 --port 5000 --peer-port 5001'"
	@echo "make test  Run transport and two-process tests (Python 3)"
	@echo "make clean Remove build products"

-include $(DEPS)
