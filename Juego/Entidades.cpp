#include "Entidades.h"

namespace {
// Fuera de la pantalla por la izquierda (la escena empieza en x = 0): ya no se ve ni vuelve.
constexpr double LIMITE_SALIDA_IZQUIERDA = -150.0;
}

Entidades::Entidades(double x, double y, double ancho, double alto)
    : x(x), y(y), ancho(ancho), alto(alto),
    vx(0.0), vy(0.0), activa(true)
{
}

void Entidades::mover(double dt)
{
    x += vx * dt;
    y += vy * dt;

    if (x + ancho < LIMITE_SALIDA_IZQUIERDA)
        activa = false;
}

bool Entidades::colisionaCon(const Entidades *otra) const
{
    if (!otra || !activa || !otra->estaActiva())
        return false;

    const bool horizontal = x < otra->getX() + otra->getAncho()
                            && x + ancho > otra->getX();
    const bool vertical = y < otra->getY() + otra->getAlto()
                          && y + alto > otra->getY();

    return horizontal && vertical;
}

double Entidades::getX() const { return x; }
double Entidades::getY() const { return y; }
double Entidades::getAncho() const { return ancho; }
double Entidades::getAlto() const { return alto; }
double Entidades::getCentroX() const { return x + ancho / 2.0; }
double Entidades::getCentroY() const { return y + alto / 2.0; }
double Entidades::getVx() const { return vx; }
double Entidades::getVy() const { return vy; }
bool Entidades::estaActiva() const { return activa; }
void Entidades::desactivar() { activa = false; }

void Entidades::setVelocidad(double nuevoVx, double nuevoVy)
{
    vx = nuevoVx;
    vy = nuevoVy;
}
