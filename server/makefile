all: SER.o
	gcc -o SER SER.o -lpthread
	rm SER.o

SER.o: SER.c
	gcc -c SER.c -lpthread -Wall `pkg-config --cflags --libs gtk+-3.0` -export-dynamic

clean:
	rm SER.o