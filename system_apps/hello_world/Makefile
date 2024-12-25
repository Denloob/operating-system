LIBC_INCLUDE := ../libc/include
LIBC := ../libc/bin/libc.a

CFLAGS := -I$(LIBC_INCLUDE) -ffreestanding -nostdlib -masm=intel -Wall -Werror -O3 -flto -static -fno-pie

.PHONY: all
all:
	gcc *.c $(LIBC) -o hello_world $(CFLAGS)

.PHONY: clean
clean:
	rm ./hello_world
