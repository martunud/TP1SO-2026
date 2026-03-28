CC = gcc
CFLAGS = -Wall -I include

all: bin/master bin/view bin/player

bin/master: src/master/master.c | bin
	$(CC) $(CFLAGS) src/master/master.c -o bin/master

bin/view: src/view/view.c | bin
	$(CC) $(CFLAGS) src/view/view.c -o bin/view

bin/player: src/player/player.c | bin
	$(CC) $(CFLAGS) src/player/player.c -o bin/player

bin:
	mkdir -p bin

clean:
	rm -f bin/master bin/view bin/player