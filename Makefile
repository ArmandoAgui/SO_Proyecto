CC ?= cc
CPPFLAGS += -Iinclude
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -g

BUILD_DIR ?= build
TARGET := $(BUILD_DIR)/dsm-node
SOURCES := src/main.c $(wildcard src/*/*.c)
OBJECTS := $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(SOURCES))
DEPS := $(OBJECTS:.o=.d)

.DEFAULT_GOAL := all
.PHONY: all run test test-unit sanitize demo docs package clean help

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(LDLIBS) -o $@

$(BUILD_DIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

run: $(TARGET)
	./$(TARGET) $(ARGS)

$(BUILD_DIR)/network-test: tests/unit/network_test.c src/network/network.c include/dsm/network.h include/dsm/types.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) tests/unit/network_test.c src/network/network.c $(LDLIBS) -o $@

$(BUILD_DIR)/memory-cli-test: tests/unit/memory_cli_test.c src/memory/memory.c src/cli/cli.c include/dsm/memory.h include/dsm/cli.h include/dsm/types.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) tests/unit/memory_cli_test.c src/memory/memory.c src/cli/cli.c $(LDLIBS) -o $@

$(BUILD_DIR)/protocol-test: tests/unit/protocol_test.c src/protocol/protocol.c src/memory/memory.c src/cli/cli.c include/dsm/protocol.h include/dsm/types.h include/dsm/memory.h include/dsm/cli.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) tests/unit/protocol_test.c src/protocol/protocol.c src/memory/memory.c src/cli/cli.c $(LDLIBS) -o $@

test-unit: $(BUILD_DIR)/network-test $(BUILD_DIR)/memory-cli-test $(BUILD_DIR)/protocol-test
	./$(BUILD_DIR)/network-test
	./$(BUILD_DIR)/memory-cli-test
	./$(BUILD_DIR)/protocol-test

test: $(TARGET) test-unit
	DSM_BINARY=$(abspath $(TARGET)) python3 -m unittest discover -s tests/integration -p 'test_*.py' -v

sanitize:
	$(MAKE) BUILD_DIR=build/sanitize CFLAGS='-std=c11 -Wall -Wextra -Wpedantic -g -fsanitize=address,undefined -fno-omit-frame-pointer' LDFLAGS='-fsanitize=address,undefined' all test-unit
	DSM_BINARY=$(abspath build/sanitize/dsm-node) python3 -m unittest discover -s tests/integration -p 'test_dsm.py' -v

demo: $(TARGET)
	DSM_BINARY=$(abspath $(TARGET)) python3 tools/run_demo.py

build/entrega/paper.pdf: docs/ieee/paper.tex
	@mkdir -p build/entrega
	pdflatex -halt-on-error -interaction=nonstopmode -output-directory=build/entrega $< > build/entrega/paper-build.txt
	pdflatex -halt-on-error -interaction=nonstopmode -output-directory=build/entrega $< >> build/entrega/paper-build.txt

build/entrega/presentacion.pdf: presentation/presentacion.tex
	@mkdir -p build/entrega
	pdflatex -halt-on-error -interaction=nonstopmode -output-directory=build/entrega $< > build/entrega/presentacion-build.txt
	pdflatex -halt-on-error -interaction=nonstopmode -output-directory=build/entrega $< >> build/entrega/presentacion-build.txt

docs: build/entrega/paper.pdf build/entrega/presentacion.pdf

package: all docs
	python3 tools/package.py

clean:
	rm -rf build

help:
	@echo "make       Compile the DSM simulator"
	@echo "make run ARGS='--id 0 --port 5000 --peer-port 5001'"
	@echo "make test  Run transport and two-process tests (Python 3)"
	@echo "make sanitize Run memory/undefined-behavior checks"
	@echo "make demo  Record verified DSM demonstration"
	@echo "make docs  Build IEEE paper and presentation PDFs (LaTeX)"
	@echo "make package Create source/PDF ZIP for delivery"
	@echo "make clean Remove build products"

-include $(DEPS)
