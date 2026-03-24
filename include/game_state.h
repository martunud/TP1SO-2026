// structs compartidos (tablero, jugadores)
#include <sys/types.h> // para pid_t
#include <stdbool.h>    // para bool

//struct dada por la cátedra
typedef struct {
    char players_name[16];        // Nombre del jugador
    unsigned int score;    // Puntaje
    unsigned int invalid_moves;    // Movimientos inválidos
    unsigned int valid_moves;    // Movimientos válidos
    unsigned short x, y; // Coordenadas x e y
    pid_t pid;           // PID
    bool blocked;            // Si está bloqueado
} player_t;

