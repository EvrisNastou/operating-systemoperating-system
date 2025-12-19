CC = gcc
RM = rm -f

all: tinyshell_2

tinyshell_2: tinyshell_2.o myfuctions.o
	$(CC) $^ -o $@

%.o: %.c
	$(CC) -c $<

run: tinyshell_2
	./tinyshell_2

clean:
	$(RM) *.o *~

purge: clean
	$(RM) tinyshell_2