EMU := qemu-system-i386
BOOTLOADER := bootloader/bootloader.bin
IMAGE_NAME := floppy

all: $(IMAGE_NAME).img

$(IMAGE_NAME).img: $(BOOTLOADER)
	cp $< $@

.PHONY: $(BOOTLOADER)
$(BOOTLOADER): 
	$(MAKE) -C bootloader

run: $(IMAGE_NAME).img
	$(EMU) -fda $<

debug: $(IMAGE_NAME).img
	bochs -f bochs_config.txt -q

clean:
	$(MAKE) clean -C bootloader

.PHONY: all run clean
