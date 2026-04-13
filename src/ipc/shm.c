#include <ipc/shm.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

static size_t game_state_size(unsigned short width, unsigned short height) {
    return sizeof(game_state_t) + width * height * sizeof(signed char);
}

static void *open_and_map_shm(const char *name, size_t size, int oflag, int prot) {
    int fd = shm_open(name, oflag, 0644);
    if (fd < 0) {
        perror("shm_open");
        return NULL;
    }

    if ((oflag & O_CREAT) && ftruncate(fd, (off_t)size) < 0) {
        perror("ftruncate");
        close(fd);
        shm_unlink(name);
        return NULL;
    }

    void *buf = mmap(NULL, size, prot, MAP_SHARED, fd, 0);
    close(fd);
    if (buf == MAP_FAILED) {
        perror("mmap");
        if (oflag & O_CREAT) {
            shm_unlink(name);
        }
        return NULL;
    }
    return buf;
}

game_state_t *create_game_shm(unsigned short width, unsigned short height) {
    if (width == 0 || height == 0) {
        fprintf(stderr, "Error: width and height must be > 0\n");
        return NULL;
    }

    unlink_game_shm();

    size_t size = game_state_size(width, height);
    game_state_t *buf = open_and_map_shm("/game_state", size, O_CREAT | O_RDWR,
                                         PROT_READ | PROT_WRITE);
    if (buf != NULL) {
        memset(buf, 0, size);
    }
    return buf;
}

sync_t *create_shm_sync(void) {
    unlink_sync_shm();

    sync_t *buf = open_and_map_shm("/game_sync", sizeof(sync_t), O_CREAT | O_RDWR,
                                   PROT_READ | PROT_WRITE);
    if (buf != NULL) {
        memset(buf, 0, sizeof(sync_t));
    }
    return buf;
}

game_state_t *open_game_shm(unsigned short width, unsigned short height) {
    game_state_t *buf = open_and_map_shm("/game_state", game_state_size(width, height),
                                         O_RDONLY, PROT_READ);
    if (buf == NULL) {
        exit(1);
    }
    return buf;
}

sync_t *open_shm_sync(void) {
    sync_t *buf = open_and_map_shm("/game_sync", sizeof(sync_t), O_RDWR,
                                   PROT_READ | PROT_WRITE);
    if (buf == NULL) {
        exit(1);
    }
    return buf;
}

int close_game_shm(game_state_t *game_state, unsigned short width, unsigned short height) {
    if (game_state == NULL) {
        return 0;
    }

    if (munmap(game_state, game_state_size(width, height)) == -1) {
        perror("munmap game_state");
        return -1;
    }
    return 0;
}

int close_shm_sync(sync_t *sync) {
    if (sync == NULL) {
        return 0;
    }

    if (munmap(sync, sizeof(sync_t)) == -1) {
        perror("munmap game_sync");
        return -1;
    }
    return 0;
}

int unlink_game_shm(void) {
    if (shm_unlink("/game_state") == -1 && errno != ENOENT) {
        perror("shm_unlink /game_state");
        return -1;
    }
    return 0;
}

int unlink_sync_shm(void) {
    if (shm_unlink("/game_sync") == -1 && errno != ENOENT) {
        perror("shm_unlink /game_sync");
        return -1;
    }
    return 0;
}
