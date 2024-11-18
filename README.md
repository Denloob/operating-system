# Operating System

Currently WIP. Supported features up to 2024-11-15 include (all custom implementations and are by us):
- 2 Stage bootloader with:
    - Dynamic physical address allocator
    - Long mode
    - MMU/Paging
    - Dynamic Kernel loader (with kernel-swapping)
- Drivers:
    - HDD/IDE drivers
    - VGA256 drivers
    - Keyboard drivers
- FAT16 filesystem
- Kernel `ptmalloc` implementation + `mmap`/`munmap`
- Image/Video rendering
- Partial libc (printf, FILE, mem/str, ...)
- Precise clock & Date clock
- Interrupt + IRQ handling

## Demo
The following video contains a technical demonstration of the current visual
features of the OS, with some joke references.
![Video of the OS running](./docs/demo.gif)

## Building

You will need `wget`, `mtools` and Make (best GNUmake) installed. \
First, (after cloning the project) install the buildtools with
```sh
make toolchain
```
After that, compile the bootloader and the kernel with
```sh
make
```

## Running

The compiled OS will appear in `hdd.img`, which contains both the bootloader
and the kernel. \
You can run the OS on any x86-64 system as long as it has BIOS support. \
One example (except running on bare metal) would be using qemu:
```sh
qemu-system-x86_64 -hda hdd.img
```

To both build and run with qemu you can use
```sh
make run
```
