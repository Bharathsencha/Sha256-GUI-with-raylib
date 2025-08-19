# Compiler
CC = gcc
CFLAGS = -Wall -O2

# Targets
TARGET_CLI = sha256_test
TARGET_GUI = sha256_gui

# Source files
CLI_SRCS = main.c sha256.c
CLI_OBJS = $(CLI_SRCS:.c=.o)

GUI_SRCS = raylib.c sha256.c
GUI_OBJS = $(GUI_SRCS:.c=.o)

# Default rule builds both
all: $(TARGET_CLI) $(TARGET_GUI)

# CLI build
$(TARGET_CLI): $(CLI_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# GUI build (needs raylib + libm)
$(TARGET_GUI): $(GUI_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -lraylib -lm

# Cleanup
clean:
	rm -f $(CLI_OBJS) $(GUI_OBJS) $(TARGET_CLI) $(TARGET_GUI)
