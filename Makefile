all: main.c
	gcc -Wall -g -fopenmp main.c -lm -o rart

clean:
	rm -f rart
