CC=gcc
CFLAGS=-O0 -g -Wall -Werror -fstack-protector-all -fsanitize=address
LIBS= -llsan

all:
	$(CC) $(CFLAGS) wumanber.c $(LIBS) -o test

clean:
	rm -rf test *~
