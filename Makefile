PROG = shell
CC = gcc
OBJS = shell.o
CFLAGS = -std=c99

$(PROG): $(OBJS)
	gcc -g -o shell $(OBJS)
shell.o: shell.c
	gcc -c $(CFLAGS) -o $(OBJS) shell.c
clean:
	rm -f core $(OBJS)
