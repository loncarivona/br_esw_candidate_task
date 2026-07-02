CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Isrc
LDFLAGS ?=

BUILD_DIR = build
RELAY_IO_SRC = src/relay_io/relay_io_sim.c

SRCS = \
	$(RELAY_IO_SRC) \
	src/relay_control/relay_instance.c \
	src/relay_control/relay_controller.c \
	src/scheduler/scheduler.c \
	src/main.c

OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))
TARGET = $(BUILD_DIR)/relay_controller_demo

.PHONY: all clean run test

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR)

TEST_BUILD_DIR = $(BUILD_DIR)/test
CMAKE ?= $(firstword $(wildcard /usr/bin/cmake /usr/bin/cmake3) cmake)
GOOGLETEST_DIR = third_party/googletest

test: $(GOOGLETEST_DIR)/CMakeLists.txt
	$(CMAKE) -S . -B $(TEST_BUILD_DIR) -DBUILD_DEMO=OFF
	$(CMAKE) --build $(TEST_BUILD_DIR)
	ctest --test-dir $(TEST_BUILD_DIR) --output-on-failure

$(GOOGLETEST_DIR)/CMakeLists.txt:
	bash scripts/fetch_googletest.sh
