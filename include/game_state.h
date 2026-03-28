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

//struct principal de la partida
typedef struct {
    unsigned short width; // Ancho del tablero
    unsigned short height; // Alto del tablero
    unsigned char players_amount; // Cantidad de jugadores
    player_t players[9]; // Lista de jugadores
    bool ended; // Indica si el juego se ha terminado
    signed char board[]; // Puntero al comienzo del tablero. fila-0, fila-1, ...
} game_state_t; 