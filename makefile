read: main.c	my_print.o
	gcc -o read my_print.o main.c
	./read

my_print.o:	my_print.asm
	nasm -f elf -o my_print.o my_print.asm

clean:
	rm my_print.o read