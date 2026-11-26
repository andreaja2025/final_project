#include "Juego.h"

Juego::Juego() : nivel(new Nivel()), iniciado(false) {}

Juego::~Juego()
{
    delete nivel;
}

void Juego::iniciar()
{
    nivel->cargar();
    iniciado = true;
}

void Juego::actualizar(double dt)
{
    if (iniciado)
        nivel->actualizar(dt);
}

void Juego::manejarEntrada(int tecla)
{
    if (iniciado)
        nivel->entradas(tecla);
}

Nivel& Juego::getNivel() { return *nivel; }
const Nivel& Juego::getNivel() const { return *nivel; }
bool Juego::estaIniciado() const { return iniciado; }
