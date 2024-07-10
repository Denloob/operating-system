.PHONY: all
all: bin/i386-elf-gcc

i386-elf-7.5.0-Linux-x86_64.tar.xz:
	wget https://newos.org/toolchains/i386-elf-7.5.0-Linux-x86_64.tar.xz

bin/i386-elf-gcc: i386-elf-7.5.0-Linux-x86_64.tar.xz
	tar -xf i386-elf-7.5.0-Linux-x86_64.tar.xz
	mv ./i386-elf-7.5.0-Linux-x86_64/* .
	rm -rf ./i386-elf-7.5.0-Linux-x86_64
