#include <master.h>
#include <shared_memory.h>
#include <view/view_utils.h>

int main(int argc, char *argv[]) {
    int exit_status = 0;
    bool finished = false;
    view_context_t ctx;
    game_state_t *buf_game = NULL;
    sync_t *buf_sync = NULL;

    if(argc !=3){
        fprintf(stderr, "Error parametros vista\n");
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

    while(true){
        int aux = sem_wait(&buf_sync->state_changed);
        if(aux == -1){
            perror("sem_wait state changed");
            exit_status = 1;
            goto cleanup;
        }

        if(view_reader_enter(buf_sync) == -1){
            perror("view_reader_enter");
            exit_status = 1;
            goto cleanup;
        }

        update_player_trail(&ctx, buf_game);
        finished = buf_game->ended;
        render_game_frame(&ctx, buf_game, finished);

        if(view_reader_exit(buf_sync) == -1){
            perror("view_reader_exit");
            exit_status = 1;
            goto cleanup;
        }

        if(sem_post(&buf_sync->view_done) == -1){
            perror("sem_post view_done");
            exit_status = 1;
            goto cleanup;
        }

        if(finished){
            if(view_reader_enter(buf_sync) == -1){
                perror("view_reader_enter");
                exit_status = 1;
                goto cleanup;
            }

            update_player_trail(&ctx, buf_game);
            render_final_frame(&ctx, buf_game);

            if(view_reader_exit(buf_sync) == -1){
                perror("view_reader_exit");
                exit_status = 1;
            }

            break;
        }
    }

cleanup:
    if(finished){
        mvprintw(height + 1, 0, "Presiona una tecla para salir...");
        refresh();
        getch();
    }
    
    destroy_view_context(&ctx);

    if(close_game_shm(buf_game, width, height) == -1){
        exit_status = 1;
    }

    if(close_shm_sync(buf_sync) == -1){
        exit_status = 1;
    }

    return exit_status;
}
