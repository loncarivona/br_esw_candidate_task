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

.PHONY: all clean run

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
