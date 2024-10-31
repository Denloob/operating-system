.PHONY: all
all: x86_64 i386
	cp -r ./x86_64/* .
	cp -r ./i386/* .

i386-elf-14.2.0-Linux-x86_64.tar.xz:
	wget https://newos.org/toolchains/i386-elf-14.2.0-Linux-x86_64.tar.xz

i386: i386-elf-14.2.0-Linux-x86_64.tar.xz
	tar -xf i386-elf-14.2.0-Linux-x86_64.tar.xz
	mv ./i386-elf-14.2.0-Linux-x86_64 i386
	rm -rf ./i386-elf-14.2.0-Linux-x86_64

x86_64-elf-14.2.0-Linux-x86_64.tar.xz:
	wget https://newos.org/toolchains/x86_64-elf-14.2.0-Linux-x86_64.tar.xz

x86_64: x86_64-elf-14.2.0-Linux-x86_64.tar.xz
	tar -xf x86_64-elf-14.2.0-Linux-x86_64.tar.xz
	mv ./x86_64-elf-14.2.0-Linux-x86_64/ x86_64
	rm -rf ./x86_64-elf-14.2.0-Linux-x86_64
