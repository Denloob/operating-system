EMU := qemu-system-x86_64
BOOTLOADER := bootloader/bootloader.bin
KERNEL := kernel/bin/kernel.bin
IMAGE_NAME := floppy

all: $(IMAGE_NAME).img

$(IMAGE_NAME).img: $(BOOTLOADER) $(KERNEL)
	cp $(BOOTLOADER) $@

	FILE_SIZE=$$(stat --format="%s" $@); \
	NEW_SIZE=$$(( (FILE_SIZE + 511) / 512 * 512 )); \
	truncate -s $$NEW_SIZE $@; \
	dd if=$(KERNEL) of=$@ bs=1 seek=$$NEW_SIZE conv=notrunc

.PHONY: $(BOOTLOADER)
$(BOOTLOADER): 
	$(MAKE) -C bootloader

.PHONY: $(KERNEL)
$(KERNEL):
	$(MAKE) -C kernel

run: $(IMAGE_NAME).img
	$(EMU) -fda $<

debug: $(IMAGE_NAME).img
	bochs -f bochs_config.txt -q

clean:
	$(MAKE) clean -C bootloader
	$(MAKE) clean -C kernel

toolchain:
	$(MAKE) -C toolchain

.PHONY: all run clean toolchain
