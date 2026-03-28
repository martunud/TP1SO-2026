// proceso vista provisorio para compilar el master
#include <stdlib.h>
#include <master.h>

int main(int argc, char *argv[]) {
    if(argc !=3){
        fprintf(stderr, "Error parametros vista\n");
        exit(1); 
    }

    unsigned short width = atoi(argv[1]); 
    unsigned short height = atoi(argv[2]);

    int shm_fd = shm_open("/game_state", O_RDONLY, 0);
    if(shm_fd < 0){
        perror("shm_open");
        exit(1);
    }

    size_t total_size = sizeof(game_state_t) + width * height * sizeof(signed char);

    game_state_t * buf = (game_state_t *) mmap(NULL, total_size, PROT_READ, MAP_SHARED, shm_fd, 0);
    if(buf == MAP_FAILED){
        perror("mmap");
        close(shm_fd);
        exit(1);
    }

    close(shm_fd);

    shm_fd = shm_open("/game_sync",  O_RDWR, 0644);
    if(shm_fd < 0){
        perror("shm_open");
        exit(1);
    }

    total_size = sizeof(sync_t);

    sync_t * bufSync = (sync_t *) mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if(bufSync == MAP_FAILED){
        perror("mmap");
        close(shm_fd);
        exit(1);
    }

    close(shm_fd);

    return 0;
}