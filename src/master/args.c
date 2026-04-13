#include <master/args.h>

int parse_args(int argc, char *argv[], unsigned short *width, unsigned short *height, unsigned int *delay, int *timeout, unsigned int *seed, char **view, char *player[MAX_PLAYERS], int *num_players ){
    int opt;
    char *endptr;
    unsigned long val;

    *width = 10;
    *height = 10;
    *delay = 200;
    *timeout = 10;
    *seed = (unsigned int) time(NULL);
    *view = NULL;
    *num_players = 0;

    while((opt = getopt(argc, argv, "w:h:d:t:s:v:p:")) != -1){
        switch(opt){
            case'w':
                val = strtoul(optarg, &endptr, 10);
                if (*endptr != '\0' || val < 10) {
                    fprintf(stderr, "Invalid integer %s\n", optarg);
                    return -1;
                }
                *width = (unsigned short) val;
                break;
            case 'h': 
                val = strtoul(optarg, &endptr, 10);
                if (*endptr != '\0'|| val < 10) {
                    fprintf(stderr, "Invalid integer %s\n", optarg);
                    return -1;
                }   
                *height = (unsigned short) val;        
                break;
            case 'd':
                val = strtoul(optarg, &endptr, 10);
                if (*endptr != '\0') {
                    fprintf(stderr, "Invalid integer %s\n", optarg);
                    return -1;
                }   
                *delay = (unsigned int) val;        
                break;
            case 't':
                val = strtoul(optarg, &endptr, 10);
                if (*endptr != '\0') {
                    fprintf(stderr, "Invalid integer %s\n", optarg); 
                    return -1;
                }   
                *timeout = (int) val;        
                break;
            case 's':
                val = strtoul(optarg, &endptr, 10);
                if (*endptr != '\0') {
                    fprintf(stderr, "Invalid integer %s\n", optarg);
                    return -1;
                }
                *seed = (unsigned int) val;
                break;
            case 'v':
                *view = optarg;
                break;
            case 'p':
                if(*num_players >= MAX_PLAYERS){
                    fprintf(stderr, "Error: at most %d players can be specified using -p.\n", MAX_PLAYERS); 
                    return -1;
                }
                player[(*num_players)++] = optarg; 
                while(optind < argc && argv[optind][0] != '-'){
                    if(*num_players >= MAX_PLAYERS){
                    fprintf(stderr, "Error: at most %d players.\n", MAX_PLAYERS);
                    return -1;
                    }
                    player[(*num_players)++] = argv[optind++];
                }
                break;
            case '?':
                fprintf(stderr, "%s: invalid option -- %c \nUsage: %s [-w width] [-h height] [-d delay] [-s seed] [-v view] [-t timeout] [-i] -p player1 player2 ...\n", argv[0], optopt, argv[0]);
                return -1;                
        }
    }
    if (*num_players == 0) {
        fprintf(stderr, "Error: At least one player must be specified using -p.\n");
        return -1;
    }
    
    return 0;
}
