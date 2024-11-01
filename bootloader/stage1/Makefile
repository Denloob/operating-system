ASM := nasm

IMAGE_NAME := boot

NASM_SOURCE := src/boot.asm

all: bin/$(IMAGE_NAME).bin

.PHONY: bin/$(IMAGE_NAME).bin # It doesn't cost much to assemble one file + we need to reassemble each time stage2 size changes. This is the easiest solution.
bin/$(IMAGE_NAME).bin: $(NASM_SOURCE)
ifndef STAGE_TWO_SIZE
	$(error stage2 bootloader size (`STAGE_TWO_SIZE`) but is required for stage1 assembling)
endif
	mkdir -p bin
	STAGE_TWO_SIZE=$(STAGE_TWO_SIZE); \
	$(ASM) $< -o $@ -f bin -D STAGE2_BOOTLOADER_SIZE_IN_SECTORS=$$(( (STAGE_TWO_SIZE + 511) / 512 ))

clean:
	rm -rf bin

.PHONY: all clean
