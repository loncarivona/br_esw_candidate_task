CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Isrc
LDFLAGS ?=

RELAY_IO_SRC = src/relay_io/relay_io_sim.c

SRCS = \
	$(RELAY_IO_SRC) \
	src/relay_control/relay_instance.c \
	src/relay_control/relay_controller.c \
	src/scheduler/scheduler.c \
	src/main.c

OBJS = $(SRCS:.c=.o)
TARGET = relay_controller_demo

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
