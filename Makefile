CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Isrc
LDFLAGS ?=

RELAY_IO_SRC = src/relay_io/relay_io_sim.c

CORE_SRCS = \
	$(RELAY_IO_SRC) \
	src/relay_control/relay_instance.c \
	src/relay_control/relay_controller.c \
	src/scheduler/scheduler.c

.PHONY: all prod debug clean run run-prod run-debug test test-cmake test-gcc

all: prod

prod:
	$(MAKE) relay_controller_demo BUILD_TYPE=prod

debug:
	$(MAKE) relay_controller_demo BUILD_TYPE=debug

ifeq ($(BUILD_TYPE),debug)
BUILD_DIR = build/debug
CFLAGS += -DLOG_ENABLED
LOG_SRC = src/relay_control/relay_log_host.c
else
BUILD_DIR = build/prod
LOG_SRC =
endif

SRCS = $(CORE_SRCS) src/main.c src/scenario/scenario_csv.c $(LOG_SRC)
OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))
TARGET = $(BUILD_DIR)/relay_controller_demo

relay_controller_demo: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

run-prod: prod
	./build/prod/relay_controller_demo

run-debug: debug
	./build/debug/relay_controller_demo

run: run-debug

clean:
	rm -rf build

# Unit tests — cmake if available, otherwise direct g++ build (no cmake required).
TEST_BUILD_DIR = build/test
TEST_GCC_DIR = build/test-gcc
CMAKE := $(shell command -v cmake 2>/dev/null || command -v cmake3 2>/dev/null)
GOOGLETEST_DIR = third_party/googletest
GTEST_DIR = $(GOOGLETEST_DIR)/googletest
GTEST_INC = $(GTEST_DIR)/include

CXX ?= g++
TEST_CXXFLAGS = -std=c++20 -Wall -Wextra -Wpedantic -Isrc -Itest/include -I$(GTEST_INC) -pthread
TEST_CXX_SRCS = \
	test/main.cpp \
	test/relay_controller_test.cpp \
	test/relay_instance_test.cpp \
	test/scheduler_test.cpp
TEST_CXX_OBJS = $(patsubst %.cpp,$(TEST_GCC_DIR)/%.o,$(TEST_CXX_SRCS))
TEST_C_OBJS = $(patsubst %.c,$(TEST_GCC_DIR)/%.o,$(CORE_SRCS))
GTEST_OBJ = $(TEST_GCC_DIR)/gtest-all.o
TEST_EXE = $(TEST_GCC_DIR)/relay_controller_tests

ifeq ($(CMAKE),)
test: test-gcc
else
test: test-cmake
endif

test-cmake: $(GOOGLETEST_DIR)/CMakeLists.txt
	$(CMAKE) -S . -B $(TEST_BUILD_DIR) -DBUILD_DEMO=OFF
	$(CMAKE) --build $(TEST_BUILD_DIR)
	ctest --test-dir $(TEST_BUILD_DIR) --output-on-failure

test-gcc: $(GOOGLETEST_DIR)/googletest/CMakeLists.txt $(TEST_EXE)
	$(TEST_EXE)

$(TEST_EXE): $(GTEST_OBJ) $(TEST_C_OBJS) $(TEST_CXX_OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $^ -o $@ $(LDFLAGS) -pthread

$(TEST_GCC_DIR)/gtest-all.o: $(GTEST_DIR)/src/gtest-all.cc
	@mkdir -p $(dir $@)
	$(CXX) $(TEST_CXXFLAGS) -I$(GTEST_DIR) -c $< -o $@

$(TEST_GCC_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(TEST_CXXFLAGS) -c $< -o $@

$(TEST_GCC_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(GOOGLETEST_DIR)/CMakeLists.txt:
	bash scripts/fetch_googletest.sh
