
all: mandel mandelmovie

mandelmovie: mandelmovie.c
	gcc mandelmovie.c -o mandelmovie -Wall -lm

mandel: mandel.o bitmap.o
	gcc mandel.o bitmap.o -o mandel -lpthread -lm

mandel.o: mandel.c
	gcc -Wall -g -c mandel.c -o mandel.o -lm

bitmap.o: bitmap.c
	gcc -Wall -g -c bitmap.c -o bitmap.o

clean:
		rm -f mandel.o bitmap.o mandel mandelmovie
