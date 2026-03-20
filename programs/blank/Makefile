# EDIT 06: This keeps the sample blank program compatible with the new x86_64 build environment used by the project.
all:
	nasm -f elf64 ./blank.asm -o ./build/blank.o
	x86_64-elf-gcc -g -T ./linker.ld -o ./blank.bin -ffreestanding -O0 -nostdlib -fpic -m64 -mno-red-zone -mcmodel=large ./build/blank.o

clean:
	rm -rf ./build/blank.o