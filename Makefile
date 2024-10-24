EMU := qemu-system-x86_64
BOOTLOADER := bootloader/bootloader.bin
KERNEL := kernel/bin/kernel.bin
IMAGE_NAME := hdd

all: $(IMAGE_NAME).img

$(IMAGE_NAME).img: $(BOOTLOADER) $(KERNEL) initfat16
	cp $(BOOTLOADER) $@

	dd if=/dev/zero of=$@ bs=1 seek=$$(stat --format="%s" $@) count=$$(printf "%d" 0x10000)

	./util/initfat16/bin/initfat16 $@

	mcopy $(KERNEL) a:

.PHONY: initfat16
initfat16:
	$(MAKE) -C ./util/initfat16

.PHONY: $(BOOTLOADER)
$(BOOTLOADER): 
	$(MAKE) -C bootloader

.PHONY: $(KERNEL)
$(KERNEL):
	$(MAKE) -C kernel

run: $(IMAGE_NAME).img
	$(EMU) -drive id=disk,file=$<,if=none -device piix3-ide,id=ide -device ide-hd,drive=disk,bus=ide.0

debug: $(IMAGE_NAME).img
	bochs -f bochs_config.txt -q

clean:
	$(MAKE) clean -C bootloader
	$(MAKE) clean -C kernel

toolchain:
	$(MAKE) -C toolchain

.PHONY: all run clean toolchain
