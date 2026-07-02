CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Isrc
LDFLAGS ?=

RELAY_IO_SRC = src/relay_io/relay_io_sim.c

CORE_SRCS = \
	$(RELAY_IO_SRC) \
	src/relay_control/relay_instance.c \
	src/relay_control/relay_controller.c \
	src/scheduler/scheduler.c

.PHONY: all prod debug clean run run-prod run-debug test

all: prod

prod:
	$(MAKE) relay_controller_demo BUILD_TYPE=prod

debug:
	$(MAKE) relay_controller_demo BUILD_TYPE=debug

ifeq ($(BUILD_TYPE),debug)
BUILD_DIR = build/debug
CFLAGS += -DRELAY_LOG_ENABLED
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

TEST_BUILD_DIR = build/test
CMAKE ?= $(firstword $(wildcard /usr/bin/cmake /usr/bin/cmake3) cmake)
GOOGLETEST_DIR = third_party/googletest

test: $(GOOGLETEST_DIR)/CMakeLists.txt
	$(CMAKE) -S . -B $(TEST_BUILD_DIR) -DBUILD_DEMO=OFF
	$(CMAKE) --build $(TEST_BUILD_DIR)
	ctest --test-dir $(TEST_BUILD_DIR) --output-on-failure

$(GOOGLETEST_DIR)/CMakeLists.txt:
	bash scripts/fetch_googletest.sh
