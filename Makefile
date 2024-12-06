EMU := qemu-system-x86_64
BOOTLOADER := bootloader/bootloader.bin
KERNEL := kernel/bin/kernel.bin
DEBUG_SYM := kernel/bin/kernel.sym
IMAGE_NAME := hdd

all: $(IMAGE_NAME).img

$(IMAGE_NAME).img: $(BOOTLOADER) $(KERNEL) initfat16 kernel.cfg
	cp $(BOOTLOADER) $@

	dd if=/dev/zero of=$@ bs=1 seek=$$(stat --format="%s" $@) count=$$(printf "%d" 0x10000)

	./util/initfat16/bin/initfat16 $@

	mcopy $(KERNEL) a:
	mcopy ./assets/video.bmp a:
	mcopy kernel.cfg a:

	dd if=/dev/zero of=$@ bs=1 seek=$$(stat --format="%s" $@) count=$$(printf "%d" 0x10000)

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
QEMU_CPU := Skylake-Client,-xsavec,-rtm,-hle,-pcid,-invpcid,-tsc-deadline
QEMU_MISC_OPTIONS := -no-reboot -monitor stdio -cpu $(QEMU_CPU)

QEMU_DEBUG_OPTIONS := -gdb tcp::1234 -S
GDB_COMMAND := gdb $(shell realpath $(DEBUG_SYM)) -ex "target remote localhost:1234"

kernel.cfg: kernel.cfg.def
	cp $< $@

run: $(IMAGE_NAME).img
	$(EMU) -hda $< $(QEMU_LOG_OPTIONS) $(QEMU_MISC_OPTIONS)

.PHONY: runb
runb: # Run, don't build
	$(EMU) -hda $(IMAGE_NAME).img $(QEMU_LOG_OPTIONS) $(QEMU_MISC_OPTIONS)

.PHONY: $(DEBUG_SYM)
$(DEBUG_SYM):
	$(MAKE) debug -C kernel

.PHONY: debug
debug: $(IMAGE_NAME).img $(DEBUG_SYM)
	@if [ -n "$$TMUX" ]; then                                                  \
		tmux new-window -n 'kernel-debug' '$(GDB_COMMAND)';                    \
	else                                                                       \
		echo -e '\n';                                                          \
		echo "Run the following command:";                                     \
		echo -e '```';                                                         \
		echo '$(GDB_COMMAND)';                                                 \
		echo -e '```\n';                                                       \
		read -p "Then, press enter to start the OS...";                        \
	fi

	$(EMU) -hda $< $(QEMU_LOG_OPTIONS) $(QEMU_MISC_OPTIONS) $(QEMU_DEBUG_OPTIONS)

clean:
	$(MAKE) clean -C bootloader
	$(MAKE) clean -C kernel

toolchain:
	$(MAKE) -C toolchain

.PHONY: all run clean toolchain
