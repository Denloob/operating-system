EMU := qemu-system-i386
BOOTLOADER := bootloader/bin/boot.bin

all: $(BOOTLOADER)

$(BOOTLOADER):
	$(MAKE) -C bootloader

run: $(BOOTLOADER)
	$(EMU) -hda $<

clean:
	$(MAKE) clean -C bootloader

.PHONY: all run clean
