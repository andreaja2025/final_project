#ifndef EVENTOJUEGO_H
#define EVENTOJUEGO_H

// Cosas que pasan en el nivel y que la capa de presentación puede querer mostrar
// (hoy: sonido). Es solo un tipo de dato: no tiene lógica ni depende de Qt.
enum EventoJuego {
    EVENTO_NIVEL_INICIADO,       // el nivel se cargó (también al reiniciar)
    EVENTO_CUENTA_REGRESIVA,     // un número de la cuenta 3-2-1
    EVENTO_YA,                   // termina la preparación y empieza la carrera
    EVENTO_CAMBIO_CARRIL,
    EVENTO_LANZAMIENTO_BOTELLA,  // el Ángel lanza una botella
    EVENTO_IMPACTO_BOTELLA,      // la botella golpea a Terminator
    EVENTO_BOTELLA_FALLADA,      // la botella cae sin golpear
    EVENTO_CAIDA_HUECO,
    EVENTO_PREMIO,               // recoge el premio (empieza el rapidín)
    EVENTO_RAPIDIN_FIN,
    EVENTO_CHOQUE_FATAL,         // choque con un obstáculo fijo
    EVENTO_NIVEL_COMPLETADO,
    EVENTO_NIVEL_PERDIDO
};

#endif // EVENTOJUEGO_H
