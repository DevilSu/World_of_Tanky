all: SER.o
	gcc -o SER SER.o `pkg-config --cflags --libs gtk+-3.0` -rdynamic
	rm SER.o

SER.o: SER.c
	gcc SER.c -lpthread -Wall `pkg-config --cflags --libs gtk+-3.0` -export-dynamic -c -o SER.o

clean:
	rm SER.o
