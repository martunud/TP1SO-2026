#ifndef MASTER_H
#define MASTER_H

#include <unistd.h> 
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/select.h>

//Para la shared memory -> en man shm_open
#include <sys/mman.h>
#include <sys/stat.h>        
#include <fcntl.h>           

#include <game_state.h>
#include <game_sync.h> 

extern char *optarg;
extern int optind;
extern int opterr;
extern int optopt;

#define MAX_PLAYERS 9

#endif