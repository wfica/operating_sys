CFLAGS = -g
LIBS = -lpthread
CC = gcc $(CFLAGS) 

filo:
	$(CC) filozof.c $(LIBS) -o filo.out


clean:
	rm -f *.out *.o
