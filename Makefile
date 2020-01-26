PROG = shell
CC = gcc
OBJS = shell.o

$(PROG): $(OBJS)
	gcc -g -o shell $(OBJS)
shell.o: shell.c
	gcc -c -o $(OBJS) shell.c
clean:
	rm -f core $(OBJS)
