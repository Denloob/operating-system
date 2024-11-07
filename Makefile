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

QEMU_LOG_OPTIONS := -d int,cpu_reset,in_asm,guest_errors -D log.txt
QEMU_MISC_OPTIONS := -no-reboot -monitor stdio
QEMU_DEBUG_OPTIONS := -gdb tcp::1234 -S

run: $(IMAGE_NAME).img
	$(EMU) -hda $< $(QEMU_LOG_OPTIONS) $(QEMU_MISC_OPTIONS)

runb: # Run, don't build
	$(EMU) -hda $(IMAGE_NAME).img $(QEMU_LOG_OPTIONS) $(QEMU_MISC_OPTIONS)

debug: $(IMAGE_NAME).img
	$(EMU) -hda $< $(QEMU_LOG_OPTIONS) $(QEMU_MISC_OPTIONS) $(QEMU_DEBUG_OPTIONS)

clean:
	$(MAKE) clean -C bootloader
	$(MAKE) clean -C kernel

toolchain:
	$(MAKE) -C toolchain

.PHONY: all run clean toolchain
