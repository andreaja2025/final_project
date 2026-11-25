#ifndef JUEGO_H
#define JUEGO_H

#include "Nivel.h"

// Propietario del nivel activo (relación 1 a 0..1 con Nivel).
class Juego
{
private:
    Nivel *nivel;
    bool iniciado;

public:
    Juego();
    ~Juego();

    Juego(const Juego&) = delete;
    Juego& operator=(const Juego&) = delete;

    void iniciar();
    void actualizar(double dt);
    void manejarEntrada(int tecla);
    Nivel& getNivel();
    const Nivel& getNivel() const;
    bool estaIniciado() const;
};

#endif // JUEGO_H
