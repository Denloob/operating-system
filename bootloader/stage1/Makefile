ASM := nasm

IMAGE_NAME := boot

NASM_SOURCE := src/boot.asm

all: bin/$(IMAGE_NAME).bin

bin/$(IMAGE_NAME).bin: $(NASM_SOURCE)
	mkdir -p bin
	$(ASM) $< -o $@ -f bin # nasm the source into the target i.e. `bin/$(IMAGE_NAME)`

clean:
	rm -rf bin

.PHONY: all clean
