all: main.c
	gcc -Wall main.c -o rart

clean:
	rm -f rart
