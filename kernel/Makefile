BIN_DIR := bin
SRC_DIR := src
DEP_DIR := .deps

IMAGE_NAME := kernel

ASM := nasm

CC := x86_64-elf-gcc
LD := x86_64-elf-ld
REDEFINED_64BIT_FLAGS := -U'__INT64_TYPE__' -D'__INT64_TYPE__=long long int' -U'__UINT64_TYPE__' -D'__UINT64_TYPE__=unsigned long long int'
CFLAGS := -ffreestanding -nostdlib -masm=intel -fno-zero-initialized-in-bss -mno-red-zone -Wall -Werror $(REDEFINED_64BIT_FLAGS) -mcmodel=kernel -O3 -g
LDFLAGS := -nostdlib -T linker.ld -Map=$(BIN_DIR)/$(IMAGE_NAME).map
LDLIBS := -L${TOOLCHAIN_PATH}/lib/gcc/x86_64-elf/14.2.0 -lgcc
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEP_DIR)/obj/$*.d

CFILES := $(shell cd $(SRC_DIR) && find -L * -type f -name '*.c')
NASMFILES := $(shell cd $(SRC_DIR) && find -L * -type f -name '*.asm')
OBJS := $(addprefix $(BIN_DIR)/obj/,$(CFILES:.c=.c.o) $(NASMFILES:.asm=.asm.o))
DEPS := $(addprefix $(DEP_DIR)/obj/,$(CFILES:.c=.d))

KERNEL_ELF := $(BIN_DIR)/$(IMAGE_NAME).elf
KERNEL_BIN := $(BIN_DIR)/$(IMAGE_NAME).bin
KERNEL_SYM := $(BIN_DIR)/$(IMAGE_NAME).sym

all: $(KERNEL_BIN)

.PHONY: debug
debug: $(KERNEL_SYM)

$(KERNEL_ELF): $(OBJS)
	$(LD) $^ $(LDFLAGS) -o $@ $(LDLIBS)

$(KERNEL_BIN): $(KERNEL_ELF)
	objcopy -O binary $< $@

$(KERNEL_SYM): $(KERNEL_ELF)
	objcopy --only-keep-debug $< $@

$(BIN_DIR)/obj/%.c.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $(OBJS)) $(dir $(DEPS)) $(BIN_DIR)
	$(CC) -c $< -o $@ $(CFLAGS) $(DEPFLAGS)

$(BIN_DIR)/obj/%.asm.o: $(SRC_DIR)/%.asm
	@mkdir -p $(dir $(OBJS)) $(BIN_DIR)
	$(ASM) -f elf -o $@ $<

-include $(DEPS)

clean:
	rm -rf $(BIN_DIR) $(DEP_DIR)

.PHONY: all clean
