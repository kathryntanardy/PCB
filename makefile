all:
	gcc -Wall -Werror -o output main.c list.o PCB.c -o output 

clean:
	rm output