all: main.c
	gcc -Wall -g main.c -o rart

clean:
	rm -f rart
