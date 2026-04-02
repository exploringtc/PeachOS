# Build configuration for the CYSE570 to-do app.
# CROSS_COMPILE lets you switch toolchains easily (example: i686-elf-).
CROSS_COMPILE ?=
CC := $(CROSS_COMPILE)gcc

TARGET := build/todo_app.elf
SRC_DIR := src
OBJ_DIR := build
PEACHOS_DIR ?= PeachOS
PROGRAMS_DIR ?= $(PEACHOS_DIR)/programs
PROGRAM_NAME ?= todo_app.elf

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

CFLAGS ?= -std=c11 -Wall -Wextra -Werror -ffreestanding -fno-builtin -fno-stack-protector
LDFLAGS ?= -nostdlib -Wl,-e,_start

.PHONY: all check install clean

all: $(TARGET)

# Syntax check across all source files without producing binaries.
check:
	$(CC) -std=c11 -Wall -Wextra -Werror -fsyntax-only $(SRCS)

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

# Installs the built user program into the PeachOS programs directory.
install: $(TARGET)
	@mkdir -p $(PROGRAMS_DIR)
	cp $(TARGET) $(PROGRAMS_DIR)/$(PROGRAM_NAME)
	@echo "installed: $(PROGRAMS_DIR)/$(PROGRAM_NAME)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)
