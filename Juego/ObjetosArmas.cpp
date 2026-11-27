#include "ObjetosArmas.h"
#include "PersonajePrincipal.h"

#include <cmath>

namespace {
// Límites del área jugable (la escena mide 1280 x 720).
constexpr double LIMITE_IZQUIERDO = -200.0;
constexpr double LIMITE_DERECHO = 1600.0;
constexpr double LIMITE_INFERIOR = 900.0;
}

ObjetosArmas::ObjetosArmas()
    : Entidades(0.0, 0.0, 0.0, 0.0), tipo(HUECO), efectoAplicado(false),
    x0(0.0), y0(0.0), vx0(0.0), vy0(0.0), tiempoVida(0.0), tiempoTranscurrido(0.0),
    carrilObjetivo(-1), resultadoPendiente(false)
{
    activa = false;
}

void ObjetosArmas::crearHueco(double x, double y, double ancho, double alto)
{
    this->x = x;
    this->y = y;
    this->ancho = ancho;
    this->alto = alto;
    vx = vy = 0.0;
    tipo = HUECO;
    efectoAplicado = false;
    carrilObjetivo = -1;
    resultadoPendiente = false;
    activa = true;
}

void ObjetosArmas::crearBotella(double x, double y, double ancho, double alto,
                                double velocidadInicial, double angulo,
                                double tiempoVida, int carrilObjetivo)
{
    this->x = x;
    this->y = y;
    this->ancho = ancho;
    this->alto = alto;
    tipo = BOTELLA;
    efectoAplicado = false;

    x0 = x;
    y0 = y;
    vx0 = velocidadInicial * std::cos(angulo);
    vy0 = velocidadInicial * std::sin(angulo);
    vx = vx0;
    vy = vy0;
    this->tiempoVida = tiempoVida;
    tiempoTranscurrido = 0.0;
    this->carrilObjetivo = carrilObjetivo;
    resultadoPendiente = true;
    activa = true;
}

void ObjetosArmas::liberar()
{
    activa = false;
    resultadoPendiente = false;
}

void ObjetosArmas::actualizar(double dt)
{
    if (!activa || dt <= 0.0)
        return;

    if (tipo == HUECO) {
        mover(dt);
        return;
    }

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

// El hueco y la botella quitan una vida; el nivel solo termina si el personaje se queda sin vidas.
void ObjetosArmas::colocarefect(PersonajePrincipal *jugador)
{
    if (!jugador || efectoAplicado || !activa)
        return;

    jugador->recibirDanio();
    efectoAplicado = true;
    desactivar();
}

void ObjetosArmas::calcularLanzamiento(double xInicial, double yInicial,
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

TipoArma ObjetosArmas::getTipo() const { return tipo; }
bool ObjetosArmas::esBotella() const { return tipo == BOTELLA; }
bool ObjetosArmas::efectoFueAplicado() const { return efectoAplicado; }
bool ObjetosArmas::huboImpacto() const { return efectoAplicado; }
int ObjetosArmas::getCarrilObjetivo() const { return carrilObjetivo; }
bool ObjetosArmas::resultadoPorRegistrar() const { return resultadoPendiente; }
void ObjetosArmas::resultadoRegistrado() { resultadoPendiente = false; }
