CC = gcc
CFLAGS = -Wall
OBJS = test.o

all: test

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

test: $(OBJS)
	$(CC) -o $@ $(OBJS)

clean:
	rm -f rbcfg *.o
