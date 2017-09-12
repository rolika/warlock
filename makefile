CC = gcc-7
CFLAGS = -std=c11 -Wall
OFLAGS = -c

# compiler process so far
# 1. gcc -o warlock warlock.c
# some explanation on gcc options:
# -I: look for .h files in this directory first
# -c: do not link, just produce .o files
# -Wall: display all kinds of warnings
# -o: link object files into this executable
# -std=c11: use certain ansi c version (c11 = 2011 - latest)

all: main.o
	$(CC) $(CFLAGS) -o warlock main.o

main.o: main.c
	$(CC) $(CFLAGS) $(OFLAGS) main.c

# notes on rm options:
# -f: ignore non-existent files and arguments without asking
# -r: recursively remove directories and their contents
# -i: ask for each remove
# -I: ask only if there are more than three removals
# -d: remove empty directories
# -v: verbose, explain what is happening
clean:
	rm -f *.o
	rm -f warlock
	rm -f player.dat
	