all: main.c
	gcc -Wall -g main.c -lm -o rart

clean:
	rm -f rart
