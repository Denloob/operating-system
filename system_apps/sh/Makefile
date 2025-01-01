BIN_DIR := bin
SRC_DIR := src
DEP_DIR := .deps

LIBC_INCLUDE := ../libc/include
LIBC := ../libc/bin/libc.a

CC = gcc
CFLAGS := -I$(LIBC_INCLUDE) -ffreestanding -nostdlib -masm=intel -Wall -Werror -flto -fno-pie -fno-stack-protector -O3 -mgeneral-regs-only
LDFLAGS := -ffreestanding -nostdlib -static -flto -static
LDLIBS := -L${TOOLCHAIN_PATH}/lib/gcc/x86_64-elf/14.2.0 -lgcc

CFILES := $(shell cd $(SRC_DIR) && find -L * -type f -name '*.c')
OBJS := $(addprefix $(BIN_DIR)/obj/,$(CFILES:.c=.c.o))
DEPS := $(addprefix $(DEP_DIR)/obj/,$(CFILES:.c=.d))

.PHONY: all
all: $(BIN_DIR)/sh

$(BIN_DIR)/sh: $(OBJS) $(LIBC)
	$(CC) $^ $(LDFLAGS) -o $@ $(LDLIBS)

$(BIN_DIR)/obj/%.c.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $(OBJS)) $(dir $(DEPS)) $(BIN_DIR)
	$(CC) -c $< -o $@ $(CFLAGS) $(DEPFLAGS)

.PHONY: clean
clean:
	rm -rf $(BIN_DIR) $(DEP_DIR)
