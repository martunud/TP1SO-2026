
#ifndef GAME_SYNC_H
#define GAME_SYNC_H

// structs de sincronización (semáforos)
#include <semaphore.h>

typedef struct {
    sem_t state_changed; // El máster le indica a la vista que hay cambios por imprimir
    sem_t view_done; // La vista le indica al máster que terminó de imprimir
    sem_t writer_mutex; // Mutex para evitar inanición del máster al acceder al estado
    sem_t state_mutex; // Mutex para el estado del juego
    sem_t readers_count_mutex; // Mutex para la siguiente variable
    unsigned int readers_count; // Cantidad de jugadores leyendo el estado
    sem_t move_processed[9]; // Le indican a cada jugador que puede enviar 1 movimiento
} sync_t;

#endif