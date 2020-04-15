#compiler used
CC = gcc

CFLAGS = -Wall

#executable name
EXECUTABLE = dkt

$(EXECUTABLE):
	$(CC) $(CFLAGS) *.c -o $(EXECUTABLE)

clean:
	rm $(EXECUTABLE)
