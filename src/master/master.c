#include <shared_memory.h>
#include <args.h>

int main (int argc, char *argv[]) {
    unsigned short width, height;
    unsigned int delay, seed;
    int timeout, num_players;
    char *view;
    char *player[MAX_PLAYERS] ;

    int parsed = parse_args(argc, argv, &width, &height, &delay, &timeout, &seed, &view, player, &num_players);

    if(parsed == -1){
        exit(1);
    }
    else{
        printf("width: %hu\nheight: %hu\ndelay: %u\ntimeout: %d\nseed: %u\n", width, height, delay, timeout, seed);
        if(view == NULL){
            printf("view: -\n");
        }else{
            printf("view: %s\n", view);
        }
        printf("num_players: %d\n", num_players);
        for(int i=0; i<num_players; i++){
            printf("  %s\n", player[i]);
        }
    }
    return 0;
}
