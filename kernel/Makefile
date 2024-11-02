BIN_DIR := bin
SRC_DIR := src
DEP_DIR := .deps

IMAGE_NAME := kernel

ASM := nasm

CC := x86_64-elf-gcc
LD := x86_64-elf-ld
CFLAGS := -ffreestanding -nostdlib -masm=intel -fno-zero-initialized-in-bss
ISRCFLAGS := -mgeneral-regs-only -mno-red-zone
LDFLAGS := -nostdlib -T linker.ld -Map=$(BIN_DIR)/$(IMAGE_NAME).map
LDLIBS := -L${TOOLCHAIN_PATH}/lib/gcc/x86_64-elf/14.2.0 -lgcc
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEP_DIR)/obj/$*.d

CFILES := $(shell cd $(SRC_DIR) && find -L * -type f -name '*.c' ! -name '*.isr.c')
ISRCFILES := $(shell cd $(SRC_DIR) && find -L * -type f -name '*.isr.c')
NASMFILES := $(shell cd $(SRC_DIR) && find -L * -type f -name '*.asm')
OBJS := $(addprefix $(BIN_DIR)/obj/,$(CFILES:.c=.c.o) $(NASMFILES:.asm=.asm.o) $(ISRCFILES:.c=.c.o))
DEPS := $(addprefix $(DEP_DIR)/obj/,$(CFILES:.c=.d))

all: $(BIN_DIR)/$(IMAGE_NAME).bin

$(BIN_DIR)/$(IMAGE_NAME).bin: $(OBJS)
	$(LD) $^ $(LDFLAGS) -o $@ $(LDLIBS)

$(BIN_DIR)/obj/%.isr.c.o: $(SRC_DIR)/%.isr.c
	@mkdir -p $(dir $(OBJS)) $(dir $(DEPS)) $(BIN_DIR)
	$(CC) -c $< -o $@ $(CFLAGS) $(ISRCFLAGS) $(DEPFLAGS)

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
