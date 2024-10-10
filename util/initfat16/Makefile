BIN_DIR := bin
SRC_DIR := src
DEP_DIR := .deps

ELF_NAME := initfat16

CC := gcc
LD := gcc
CFLAGS := -I${LIB_PATH}
LDFLAGS :=
LDLIBS :=
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEP_DIR)/obj/$*.d

CFILES := $(shell cd $(SRC_DIR) && find -L * -type f -name '*.c')
NASMFILES := $(shell cd $(SRC_DIR) && find -L * -type f -name '*.asm')
OBJS := $(addprefix $(BIN_DIR)/obj/,$(CFILES:.c=.c.o))
DEPS := $(addprefix $(DEP_DIR)/obj/,$(CFILES:.c=.d))

all: $(BIN_DIR)/$(ELF_NAME)

$(BIN_DIR)/$(ELF_NAME): $(OBJS)
	$(LD) $^ $(LDFLAGS) -o $@ $(LDLIBS)

$(BIN_DIR)/obj/%.c.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $(OBJS)) $(dir $(DEPS)) $(BIN_DIR)
	$(CC) -c $< -o $@ $(CFLAGS) $(DEPFLAGS)

-include $(DEPS)

clean:
	rm -rf $(BIN_DIR) $(DEP_DIR)

.PHONY: all clean
