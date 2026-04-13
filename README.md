# TP1SO-2026 - ChompChamps

## Design Decisions

The game runs with three types of processes: a master, a view, and one or more players. Communication between the master and each player is handled through anonymous pipes: the player writes its move to `stdout`, which was redirected to the write end of the pipe before `execv`. The master reads from the read end and applies the move to the shared state.

The game state and inter-process synchronization are managed through two POSIX shared memory segments (`/game_state` and `/game_sync`). The game segment contains a flexible array member for the board, so its total size is `sizeof(game_state_t) + width * height * sizeof(signed char)`, avoiding an unnecessary indirection. All processes access this segment concurrently, so the critical section is protected with a classic readers-writer scheme that favors readers and prevents writer starvation through an additional `writer_mutex` semaphore.

Synchronization between the master and the view is handled with two semaphores: `state_changed` (the master signals that a new frame is ready) and `view_done` (the view confirms it has finished rendering). Each player also has its own `move_processed[i]` semaphore that the master uses to enable them one at a time in round-robin order, preserving the start index between iterations so the first players are not always favored.

The initial placement of players divides the board into equal horizontal sectors and positions each player at the center of its sector, searching for the nearest free cell by increasing radius. This ensures each player starts with a similar margin of space.

The player strategy implements a one-step greedy algorithm: it prefers the neighboring cell with the highest value that also has the greatest number of future moves available. If no candidate has future exits, it falls back to simply choosing the cell with the highest immediate value.

When no view is provided with `-v`, the master skips view notifications and `view_pid` is set to `-1`, allowing the game to run in headless mode.

## Build and Run

The project compiles inside a multiarch Docker container (ARM or AMD). The steps are:

```bash
# From the host machine - choose according to architecture:
make dockerarm
# or
make dockeramd

# Inside the container, compile:
make build

# Run with view, 2 players, 20x20 board, 10s timeout:
make run-master P=2 W=20 H=20

# Run without view:
make run-master-noview P=2

# The binary can also be invoked directly:
./bin/master -w 20 -h 20 -t 10 -d 200 -v ./bin/view -p ./bin/player ./bin/player
```

The configurable variables for `make run-master` are `W` (width), `H` (height), `P` (number of players), `D` (delay between frames in ms), `T` (timeout in seconds), and `S` (optional random seed).

## Tournament Paths

- **View:** `./bin/view`
- **Player:** `./bin/player`

## Limitations

The view performs a full redraw of the board on every update. This approach is simple and robust, but not the most efficient: every frame repaints the entire grid even if most cells have not changed. Additionally, the interactive render depends on `ncurses`, so it is tied to a compatible terminal environment.

Error handling is mostly fail-fast: if a `fork`, `exec`, `pipe`, `mmap`, `sem_wait`, or other critical operation fails, the program does not attempt sophisticated recovery but instead terminates or aborts the current operation. This considerably simplifies the code, but does not provide strong tolerance for partial failures.

Regarding the player strategy, the algorithm is one-step greedy: it prioritizes immediate reward and some local continuity, but does not implement deep search or global board evaluation. In situations where anticipating other players' moves would be decisive, this limitation becomes relevant. Finally, the project supports up to 9 players, a limit imposed by the fixed size of the shared data structures.

## Problems Encountered During Development

One of the main problems was synchronizing the shared state. Since both players and the view need to read the board while the master may be updating it, it was necessary to implement a readers-writer protocol using `writer_mutex`, `state_mutex`, `readers_count_mutex`, and `readers_count`. The chosen solution prevents the master from being blocked indefinitely by a continuous stream of new readers.

Another problem was the initial coupling between game logic, shared memory, process creation, and synchronization. This was addressed by progressively splitting the code into more focused modules: logic that was originally concentrated in large files like `shared_memory.c` ended up divided between IPC, game rules, and master initialization and cleanup. This made individual components easier to reason about and reduced the error surface.

The handling of blocked players or players with no valid moves also required attention. The `blocked` state is recalculated after the board is initialized and after each processed move, allowing the master to stop considering players that can no longer advance without any extra communication.

A specific issue was the installation of `libncurses-dev` inside the container, since the base image did not include it. This was resolved by adding the installation to `entrypoint.sh` so it runs automatically when the environment is started. Another problem was a race condition in player PID initialization: the PID is written to shared memory from the child process just before `execv`, ensuring that `find_player_index()` can locate it correctly without depending on the operating system's scheduling order. It was also found that all pipe ends belonging to other players must be explicitly closed in the child process before `execv`; otherwise they remain open as inherited file descriptors and the master never detects EOF when reading from them.

The project was designed to be compiled inside the container provided by the course. A `Makefile` was maintained throughout development with targets to start the environment, compile the three executables, and clean intermediate artifacts. The main targets are `make dockerarm` or `make dockeramd` to enter the container, `make build` to compile, `make clean` to remove binaries and object files, and `make clean-shm` to clean residual shared memory from `/dev/shm`. It was also found that stray `.o` files could appear in the project root, so the `clean` target was updated to remove both the `.obj/` directory and any object files accidentally generated in the main directory.

## AI Usage

AI (Claude) was used as an assistant during development for specific queries about the behavior of POSIX system calls (`sem_init`, `mmap`, `select`) and to review the readers-writer synchronization logic. The source code was written and reviewed by the team; the AI did not generate blocks of code that were incorporated directly into the repository without modification.