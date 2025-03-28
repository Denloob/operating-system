EMU := qemu-system-x86_64
BOOTLOADER := bootloader/bootloader.bin
KERNEL := kernel/bin/kernel.bin
DEBUG_SYM := kernel/bin/kernel.sym
IMAGE_NAME := hdd

all: $(IMAGE_NAME).img

$(IMAGE_NAME).img: $(BOOTLOADER) $(KERNEL) sysapps initfat16 kernel.cfg
	cp $(BOOTLOADER) $@

	dd if=/dev/zero of=$@ bs=1 seek=$$(stat --format="%s" $@) count=$$(printf "%d" 0x10000)

	./util/initfat16/bin/initfat16 $@

	dd if=/dev/zero of=$@ bs=$$((1024 * 1024)) seek=$$(stat --format="%s" $@) count=32 # 32MB

	mmd /boot
	mcopy $(KERNEL) a:/boot

	mmd /boot/logo && mcd /boot/logo
	mcopy ./assets/cogs.bmp a:
	mcopy ./assets/cogs-parallel.bmp a:cogs-par.bmp
	mcopy ./assets/amongos.bmp a:

	mmd /usr
	mmd /usr/share && mcd /usr/share
	mcopy ./assets/win-desktop.bmp a:win-bg.bmp

	mmd /boot/conf && mcd /boot/conf
	mcopy kernel.cfg a:

	mmd /bin && mcd /bin
	mcopy ./system_apps/init/init     a:
	mcopy ./system_apps/mkdir/mkdir   a:
	mcopy ./system_apps/cat/cat       a:
	mcopy ./system_apps/echo/echo     a:
	mcopy ./system_apps/reboot/reboot a:
	mcopy ./system_apps/sh/bin/sh     a:
	mcopy ./system_apps/ls/ls         a:
	mcopy ./system_apps/ps/ps         a:
	mcopy ./system_apps/screensaver/scr         a:
	mcopy ./system_apps/tetris/bin/tetris      	a:
	mcopy ./system_apps/pong/bin/pong           a:
	mcopy ./system_apps/paint/bin/paint           a:
	mcopy ./system_apps/desktop/bin/mdsktop     a:

	mmd a:/dev

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
QEMU_NETWORKING := -nic tap,ifname=tap0,model=rtl8139,script=no,downscript=no
QEMU_MISC_OPTIONS := -no-reboot -monitor stdio -cpu $(QEMU_CPU) $(QEMU_NETWORKING)

QEMU_DEBUG_OPTIONS := -gdb tcp::1234 -S
GDB_COMMAND := gdb $(shell realpath $(DEBUG_SYM)) -ex "target remote localhost:1234"

kernel.cfg: kernel.cfg.def
	cp $< $@

.PHONY: network_init
network_init:
	@if ! ip link show tap0 >/dev/null 2>&1; then \
		sudo ip tuntap add dev tap0 mode tap; \
		sudo ip link set dev tap0 up; \
	fi

run: $(IMAGE_NAME).img network_init
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
		echo; echo;                                                            \
		echo "Run the following command:";                                     \
		echo '```';                                                            \
		echo '$(GDB_COMMAND)';                                                 \
		echo '```'; echo;                                                      \
		echo -n "Then, press enter to start the OS...";                        \
		read tmp;                                                                  \
	fi

	$(EMU) -hda $< $(QEMU_LOG_OPTIONS) $(QEMU_MISC_OPTIONS) $(QEMU_DEBUG_OPTIONS)

.PHONY: sysapps
sysapps:
	$(MAKE) -C system_apps

clean:
	$(MAKE) clean -C bootloader
	$(MAKE) clean -C kernel
	$(MAKE) clean -C system_apps

toolchain:
	$(MAKE) -C toolchain

.PHONY: all run clean toolchain
