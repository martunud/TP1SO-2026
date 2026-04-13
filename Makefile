DOCKER_IMAGE := agodio/itba-so-multiarch:3.1

CC      := gcc
CFLAGS  := -Wall -I include
LDFLAGS :=
LDLIBS  := -lrt

ROOT   := .
SRCDIR := $(ROOT)/src
BINDIR := $(ROOT)/bin
OBJDIR := $(ROOT)/.obj

MASTER_SRC := $(SRCDIR)/master/master.c \
	              $(SRCDIR)/master/args.c \
	              $(SRCDIR)/master/board_init.c \
	              $(SRCDIR)/master/finalize.c \
	              $(SRCDIR)/master/game.c \
	              $(SRCDIR)/master/game_loop.c \
	              $(SRCDIR)/master/results.c \
	              $(SRCDIR)/ipc/proc.c \
	              $(SRCDIR)/ipc/shm.c \
	              $(SRCDIR)/ipc/sync.c
MASTER_OBJ := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(MASTER_SRC))

VIEW_SRC := $(SRCDIR)/view/view.c \
	            $(SRCDIR)/view/view_loop.c \
	            $(SRCDIR)/view/view_utils.c \
	            $(SRCDIR)/ipc/shm.c \
	            $(SRCDIR)/ipc/sync.c
VIEW_OBJ := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(VIEW_SRC))

PLAYER_SRC := $(SRCDIR)/player/player.c \
	              $(SRCDIR)/player/runtime.c \
	              $(SRCDIR)/player/strategy.c \
	              $(SRCDIR)/ipc/shm.c \
	              $(SRCDIR)/ipc/sync.c
PLAYER_OBJ := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(PLAYER_SRC))

MASTER_BIN := $(BINDIR)/master
VIEW_BIN   := $(BINDIR)/view
PLAYER_BIN := $(BINDIR)/player

W ?= 10
H ?= 10
P ?= 2
D ?= 200
T ?= 10
S ?=

PLAYER_LIST := $(wordlist 1,$(P),./bin/player ./bin/player ./bin/player ./bin/player ./bin/player ./bin/player ./bin/player ./bin/player ./bin/player)

.DEFAULT_GOAL := help

dockerarm:
	docker run -v "$(CURDIR):/workspace" -v "$(CURDIR)/entrypoint.sh:/entrypoint.sh" -w /workspace --privileged -ti --entrypoint /bin/bash $(DOCKER_IMAGE) /entrypoint.sh

dockeramd:
	docker run -v "$(CURDIR):/workspace" -v "$(CURDIR)/entrypoint.sh:/entrypoint.sh" -w /workspace --privileged -ti --entrypoint /bin/bash $(DOCKER_IMAGE) /entrypoint.sh

ensure-docker:
	@test -f /.dockerenv || (echo "Use 'make dockerarm' or 'make dockeramd' first, then run this target inside the container." && exit 1)

all: build

build: ensure-docker dirs master view player

master: $(MASTER_BIN)

view: $(VIEW_BIN)

player: $(PLAYER_BIN)

$(MASTER_BIN): $(MASTER_OBJ) | dirs
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

$(VIEW_BIN): $(VIEW_OBJ) | dirs
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS) -lncurses

$(PLAYER_BIN): $(PLAYER_OBJ) | dirs
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | dirs
	$(CC) $(CFLAGS) -c $< -o $@

dirs:
		@mkdir -p $(BINDIR) $(OBJDIR) $(OBJDIR)/ipc $(OBJDIR)/master $(OBJDIR)/player $(OBJDIR)/view

clean:
	@rm -rf $(OBJDIR) $(MASTER_BIN) $(VIEW_BIN) $(PLAYER_BIN) ./*.o
	@echo "Clean."

clean-shm: ensure-docker
	@echo "Cleaning shared memory..."
	@-rm -f /dev/shm/game_state /dev/shm/game_sync
	@echo "Shared memory cleaned."

run-master: ensure-docker build
	@test "$(P)" -ge 1 && test "$(P)" -le 9 || (echo "P must be between 1 and 9" && exit 1)
	@test "$(H)" -ge "$(P)" || (echo "H must be >= P" && exit 1)
	@echo "MASTER=$(MASTER_BIN)"
	@echo "VIEW  =$(VIEW_BIN)"
	@echo "P=$(P)"
	@echo "PLAYER_LIST=$(PLAYER_LIST)"
	@echo "D=$(D)"
	@echo "T=$(T)"
	@echo "S=$(S)"
	$(MASTER_BIN) -w $(W) -h $(H) -d $(D) -t $(T) $(if $(S),-s $(S),) -v $(VIEW_BIN) -p $(PLAYER_LIST)

run-master-noview: ensure-docker build
	@test "$(P)" -ge 1 && test "$(P)" -le 9 || (echo "P must be between 1 and 9" && exit 1)
	@test "$(H)" -ge "$(P)" || (echo "H must be >= P" && exit 1)
	@echo "MASTER=$(MASTER_BIN)"
	@echo "P=$(P)"
	@echo "PLAYER_LIST=$(PLAYER_LIST)"
	@echo "D=$(D)"
	@echo "T=$(T)"
	@echo "S=$(S)"
	$(MASTER_BIN) -w $(W) -h $(H) -d $(D) -t $(T) $(if $(S),-s $(S),) -p $(PLAYER_LIST)

run: run-master

help:
	@echo "Expected usage:"
	@echo "  1. From the host machine: make dockerarm   or   make dockeramd"
	@echo "  2. Once inside the container: make build"
	@echo "  3. To run: make run-master P=2"
	@echo ""
	@echo "Targets:"
	@echo "  build             - Builds master, view, and player"
	@echo "  run-master        - Runs the local master with view"
	@echo "  run-master-noview - Runs the local master without view"
	@echo "  run               - Alias for run-master"
	@echo "  clean             - Removes binaries and object files"
	@echo "  clean-shm         - Removes shared memory from /dev/shm"
	@echo ""
	@echo "Configurable variables:"
	@echo "  W=10              - Board width"
	@echo "  H=10              - Board height"
	@echo "  P=2               - Number of players"
	@echo "  D=200             - Delay between moves in ms"
	@echo "  T=10              - Timeout in seconds"
	@echo "  S=                - Random seed"
	@echo ""
	@echo "Examples:"
	@echo "  make build"
	@echo "  make run-master P=2"
	@echo "  make run-master P=3 W=12 H=12"
	@echo "  make run-master-noview P=2 S=123"

.PHONY: all build master view player dirs clean clean-shm run run-master run-master-noview help dockerarm dockeramd ensure-docker
