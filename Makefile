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
              $(SRCDIR)/master/shared_memory.c \
              $(SRCDIR)/master/create_processes.c
MASTER_OBJ := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(MASTER_SRC))

VIEW_SRC := $(SRCDIR)/view/view.c \
            $(SRCDIR)/view/view_utils.c \
            $(SRCDIR)/master/shared_memory.c
VIEW_OBJ := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(VIEW_SRC))

PLAYER_SRC := $(SRCDIR)/player/player.c \
              $(SRCDIR)/master/shared_memory.c
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
	@mkdir -p $(BINDIR) $(OBJDIR) $(OBJDIR)/master $(OBJDIR)/player $(OBJDIR)/view

clean:
	@rm -rf $(OBJDIR) $(MASTER_BIN) $(VIEW_BIN) $(PLAYER_BIN)
	@echo "Limpio."

clean-shm: ensure-docker
	@echo "Limpiando memoria compartida..."
	@-rm -f /dev/shm/game_state /dev/shm/game_sync
	@echo "Memoria compartida limpiada."

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
	@echo "Uso esperado:"
	@echo "  1. Desde la maquina host: make dockerarm   o   make dockeramd"
	@echo "  2. Ya dentro del contenedor: make build"
	@echo "  3. Para correr: make run-master P=2"
	@echo ""
	@echo "Targets:"
	@echo "  build             - Compila master, view y player"
	@echo "  run-master        - Ejecuta el master propio con vista"
	@echo "  run-master-noview - Ejecuta el master propio sin vista"
	@echo "  run               - Alias de run-master"
	@echo "  clean             - Limpia binarios y objetos"
	@echo "  clean-shm         - Limpia memoria compartida en /dev/shm"
	@echo ""
	@echo "Variables configurables:"
	@echo "  W=10              - Ancho del tablero"
	@echo "  H=10              - Alto del tablero"
	@echo "  P=2               - Cantidad de jugadores"
	@echo "  D=200             - Delay entre movimientos en ms"
	@echo "  T=10              - Timeout en segundos"
	@echo "  S=                - Semilla"
	@echo ""
	@echo "Ejemplos:"
	@echo "  make build"
	@echo "  make run-master P=2"
	@echo "  make run-master P=3 W=12 H=12"
	@echo "  make run-master-noview P=2 S=123"

.PHONY: all build master view player dirs clean clean-shm run run-master run-master-noview help dockerarm dockeramd ensure-docker
