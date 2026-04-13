# TP1SO-2026 — ChompChamps

## Design Decisions
- **Player distribution**: evenly spaced horizontal sectors to give each player a similar starting margin.
- **Round-robin**: `player_start` is preserved between iterations so player 0 is not always favored.
- **Readers-writer sync**: the implementation uses a writer-starvation-safe variant with `writer_mutex`, `state_mutex`, `readers_count_mutex`, and `readers_count`.
- **Headless mode**: if `-v` is not provided, the master skips view notifications.

## Build and Run
# Open the container (ARM or AMD depending on your architecture):
make dockerarm   # or make dockeramd

# Inside the container:
make
./bin/master -w 20 -h 20 -t 10 -v ./bin/view -p ./bin/player ./bin/player

## Tournament Paths
- View:    ./bin/view
- Player:  ./bin/player

## Limitations
- The player does not implement deep lookahead; it uses a 1-step greedy strategy.

## Known Issues
- 
