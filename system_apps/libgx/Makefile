CC := x86_64-elf-gcc
AR := ar

BIN_DIR := bin
SRC_DIR := src
DEP_DIR := .deps
INCLUDE_DIR := include
LIBC_INCLUDE := ../libc/include

CFLAGS := -I$(LIBC_INCLUDE) -I$(INCLUDE_DIR) -ffreestanding -nostdlib -masm=intel -Wall -Werror -O3 -g
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEP_DIR)/obj/$*.d

CFILES := $(shell cd $(SRC_DIR) && find -L * -type f -name '*.c')
OBJS := $(addprefix $(BIN_DIR)/obj/,$(CFILES:.c=.c.o))
DEPS := $(addprefix $(DEP_DIR)/obj/,$(CFILES:.c=.d))

.PHONY: all
all: $(BIN_DIR)/libgx.a

.PHONY: clean
clean:
	rm -rf $(BIN_DIR) $(DEP_DIR)

$(BIN_DIR)/libgx.a: $(OBJS)
	$(AR) rcs $@ $^

$(BIN_DIR)/obj/%.c.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $(OBJS)) $(dir $(DEPS)) $(BIN_DIR)
	$(CC) -c $< -o $@ $(CFLAGS) $(DEPFLAGS)
