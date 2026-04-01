CC = gcc
CFLAGS = -Wall -I include

all: bin/master bin/view bin/player

bin/master: src/master/master.c src/master/args.c src/master/shared_memory.c src/master/create_processes.c | bin
	$(CC) $(CFLAGS) src/master/master.c src/master/args.c src/master/shared_memory.c src/master/create_processes.c -o bin/master

bin/view: src/view/view.c src/master/shared_memory.c | bin
	$(CC) $(CFLAGS) src/view/view.c src/master/shared_memory.c -o bin/view

bin/player: src/player/player.c src/master/shared_memory.c | bin
	$(CC) $(CFLAGS) src/player/player.c src/master/shared_memory.c -o bin/player

bin:
	mkdir -p bin

clean:
	rm -f bin/master bin/view bin/player