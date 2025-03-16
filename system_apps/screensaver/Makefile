LIBC_INCLUDE := ../libc/include
LIBC := ../libc/bin/libc.a

LIBGX_INCLUDE := ../libgx/include
LIBGX := ../libgx/bin/libgx.a

CFLAGS := -I$(LIBGX_INCLUDE) -I$(LIBC_INCLUDE) -ffreestanding -nostdlib -masm=intel -Wall -Werror -g -flto -static -fno-pie -fno-stack-protector

.PHONY: all
all:
	gcc *.c $(LIBC) $(LIBGX) -o scr $(CFLAGS)

.PHONY: clean
clean:
	rm ./scr
