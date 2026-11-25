#include "Proyectil.h"
#include "PersonajePrincipal.h"

#include <cmath>

namespace {
// Límites del área jugable (la escena mide 1280 x 720).
constexpr double LIMITE_IZQUIERDO = -200.0;
constexpr double LIMITE_DERECHO = 1600.0;
constexpr double LIMITE_INFERIOR = 900.0;
}

Proyectil::Proyectil(double x, double y, double ancho, double alto,
                     double velocidadInicial, double angulo,
                     bool botella, double tiempoVida)
    : Entidades(x, y, ancho, alto),
    botella(botella), x0(x), y0(y),
    vx0(velocidadInicial * std::cos(angulo)),
    vy0(velocidadInicial * std::sin(angulo)),
    tiempoVida(tiempoVida), tiempoTranscurrido(0.0),
    carrilObjetivo(-1), impacto(false)
{
    vx = vx0;
    vy = vy0;
}

void Proyectil::actualizar(double dt)
{
    if (!activa || dt <= 0.0)
        return;

    tiempoTranscurrido += dt;

    // Movimiento parabólico: x = x0 + vx0*t ; y = y0 + vy0*t + g*t^2/2.
    x = x0 + vx0 * tiempoTranscurrido;
    y = y0 + vy0 * tiempoTranscurrido
        + 0.5 * GRAVEDAD * tiempoTranscurrido * tiempoTranscurrido;

    // Velocidad instantánea (útil para orientar el sprite): vy = vy0 + g*t.
    vx = vx0;
    vy = vy0 + GRAVEDAD * tiempoTranscurrido;

    if (tiempoTranscurrido >= tiempoVida
        || x < LIMITE_IZQUIERDO || x > LIMITE_DERECHO || y > LIMITE_INFERIOR)
        desactivar();
}

void Proyectil::colocarefect(PersonajePrincipal *jugador)
{
    if (!jugador || impacto || !activa)
        return;

    // La botella quita una vida; el nivel solo termina si el personaje se queda sin vidas.
    jugador->recibirDanio();
    impacto = true;
    desactivar();
}

void Proyectil::calcularLanzamiento(double xInicial, double yInicial,
                                    double xObjetivo, double yObjetivo,
                                    double tiempoVuelo,
                                    double &velocidadInicial, double &angulo)
{
    if (tiempoVuelo <= 0.0) {
        velocidadInicial = 0.0;
        angulo = 0.0;
        return;
    }

    const double vxInicial = (xObjetivo - xInicial) / tiempoVuelo;
    const double vyInicial = (yObjetivo - yInicial
                              - 0.5 * GRAVEDAD * tiempoVuelo * tiempoVuelo)
                             / tiempoVuelo;

    velocidadInicial = std::hypot(vxInicial, vyInicial);
    angulo = std::atan2(vyInicial, vxInicial);
}

bool Proyectil::esBotella() const { return botella; }
bool Proyectil::huboImpacto() const { return impacto; }
void Proyectil::setCarrilObjetivo(int carril) { carrilObjetivo = carril; }
int Proyectil::getCarrilObjetivo() const { return carrilObjetivo; }
