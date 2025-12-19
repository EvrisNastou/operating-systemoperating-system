CC = gcc
RM = rm -f

all: tinyshell

tinyshell: tinyshell.o myfuctions.o
	$(CC) $^ -o $@

%.o: %.c
	$(CC) -c $<

run: tinyshell
	./tinyshell

clean:
	$(RM) *.o *~

purge: clean
	$(RM) tinyshell