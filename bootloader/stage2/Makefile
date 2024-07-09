BIN_DIR := bin
SRC_DIR := src
DEP_DIR := .deps

ASM := nasm

CC16 := wcc
LD16 := wlink
CFLAGS16 := -d3 -s -wx -ms -zl -zq

GCC := gcc
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEP_DIR)/obj/$*.d -E

CFILES := $(shell cd $(SRC_DIR) && find -L * -type f -name '*.c')
NASMFILES := $(shell cd $(SRC_DIR) && find -L * -type f -name '*.asm')
OBJS := $(addprefix $(BIN_DIR)/obj/,$(CFILES:.c=.c.o) $(NASMFILES:.asm=.asm.o))
DEPS := $(addprefix $(DEP_DIR)/obj/,$(CFILES:.c=.d))

IMAGE_NAME := boot2

all: $(BIN_DIR)/$(IMAGE_NAME).bin

$(BIN_DIR)/$(IMAGE_NAME).bin: $(OBJS)
	$(LD16) NAME $@ FILE \{ $(OBJS) \} OPTION MAP=$(BIN_DIR)/$(IMAGE_NAME).map @linker.lnk

$(BIN_DIR)/obj/%.c.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $(OBJS)) $(dir $(DEPS)) $(BIN_DIR)
	$(GCC) $< $(DEPFLAGS) > /dev/null # Generate the dep files (doesn't compile)
	$(CC16) $(CFLAGS16) -fo=$@ $<

$(BIN_DIR)/obj/%.asm.o: $(SRC_DIR)/%.asm
	@mkdir -p $(dir $(OBJS)) $(BIN_DIR)
	$(ASM) -f obj -o $@ $<

-include $(DEPS)

clean:
	rm -rf $(BIN_DIR) $(DEP_DIR)

.PHONY: all clean
