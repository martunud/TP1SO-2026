#include <stdio.h>
#include <stdlib.h>

#include <ipc/shm.h>
#include <view/view_loop.h>
#include <view/view_utils.h>

int main(int argc, char *argv[]) {
    int exit_status = 0;
    view_context_t ctx;
    game_state_t *buf_game = NULL;
    sync_t *buf_sync = NULL;

    if(argc !=3){
        fprintf(stderr, "Error: invalid view arguments\n");
        return 1;
    }

    unsigned short width = atoi(argv[1]); 
    unsigned short height = atoi(argv[2]);

    buf_game = open_game_shm(width,height);
    buf_sync = open_shm_sync();

    if(init_view_context(&ctx, width, height) == -1){
        exit_status = 1;
        goto cleanup;
    }

    if (run_view_loop(&ctx, buf_game, buf_sync) == -1) {
        exit_status = 1;
    }

cleanup:
    
    destroy_view_context(&ctx);

    if(close_game_shm(buf_game, width, height) == -1){
        exit_status = 1;
    }

    if(close_shm_sync(buf_sync) == -1){
        exit_status = 1;
    }

    return exit_status;
}
