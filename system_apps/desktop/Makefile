BIN_DIR := bin
SRC_DIR := src
DEP_DIR := .deps

LIBC_INCLUDE := ../libc/include
LIBC := ../libc/bin/libc.a

LIBGX_INCLUDE := ../libgx/include
LIBGX := ../libgx/bin/libgx.a

CC := gcc

CFLAGS := -I$(LIBGX_INCLUDE) -I$(LIBC_INCLUDE) -ffreestanding -nostdlib -masm=intel -Wall -Werror -g -flto -fno-pie -fno-stack-protector -DDEBUG
LDFLAGS := -ffreestanding -nostdlib -static -flto
LDLIBS :=

CFILES := $(shell cd $(SRC_DIR) && find -L * -type f -name '*.c')
OBJS := $(addprefix $(BIN_DIR)/obj/,$(CFILES:.c=.c.o))
DEPS := $(addprefix $(DEP_DIR)/obj/,$(CFILES:.c=.d))

.PHONY: all
all: $(BIN_DIR)/mdsktop

$(BIN_DIR)/mdsktop: $(OBJS) $(LIBC) $(LIBGX)
	@mkdir -p $(BIN_DIR)
	$(CC) $^ $(LDFLAGS) -o $@ $(LDLIBS)

$(BIN_DIR)/obj/%.c.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@) $(DEP_DIR)/obj/$(dir $*)
	$(CC) -c $< -o $@ $(CFLAGS) -MMD -MP -MF $(DEP_DIR)/obj/$*.d

-include $(DEPS)

.PHONY: clean
clean:
	rm -rf $(BIN_DIR) $(DEP_DIR)
