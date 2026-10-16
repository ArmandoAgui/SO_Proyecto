CC ?= cc
CPPFLAGS += -Iinclude
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -g

TARGET := build/dsm-node
SOURCES := src/main.c $(wildcard src/*/*.c)
OBJECTS := $(patsubst src/%.c,build/%.o,$(SOURCES))
DEPS := $(OBJECTS:.o=.d)

.DEFAULT_GOAL := all
.PHONY: all run clean help

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(LDLIBS) -o $@

build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf build

help:
	@echo "make       Compile the initial C scaffold"
	@echo "make run   Run the placeholder (no DSM simulation)"
	@echo "make clean Remove build products"

-include $(DEPS)
