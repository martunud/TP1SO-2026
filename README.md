# TP1SO-2026 — ChompChamps

## Decisiones de diseño
- **Distribución de jugadores**: sectores horizontales equidistantes para garantizar margen similar.
- **Round-robin**: se mantiene `player_start` entre iteraciones para no favorecer siempre al jugador 0.
- **Lectores-escritores**: se implementó la variante que previene inanición del escritor (master) usando `writer_mutex`, `state_mutex`, `readers_count_mutex` y `readers_count`.
- **Sin vista**: si no se pasa `-v`, el master omite las notificaciones a la vista.

## Compilación y ejecución
# Abrir el contenedor (ARM o AMD según tu arquitectura):
make dockerarm   # o make dockeramd

# Dentro del contenedor:
make
./bin/master -w 20 -h 20 -t 10 -v ./bin/view -p ./bin/player ./bin/player

## Rutas para el torneo
- Vista:    ./bin/view
- Jugador:  ./bin/player

## Limitaciones
- El jugador no implementa lookahead profundo; toma decisiones greedy a 1 paso.

## Problemas encontrados
- 