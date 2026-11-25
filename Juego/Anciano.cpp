#include "Anciano.h"
#include "Proyectil.h"

#include <algorithm>
#include <cmath>

Anciano::Anciano(double x, double y, double ancho, double alto,
                 double tiempoEntreLanzamiento)
    : Entidades(x, y, ancho, alto),
    tiempoEntreLanzamiento(tiempoEntreLanzamiento),
    temporizador(0.0), angulo(0.0), velocidadInicial(0.0), tiempoVuelo(1.25),
    velocidadVertical(260.0), alturaSobreCarril(60.0),
    carrilJugador(1), carrilObjetivo(1), yObjetivo(y), decisionTomada(false),
    tiempoDesdeLanzamiento(0.0), lanzamientoActivo(false)
{
    for (int i = 0; i < 3; ++i) {
        centroCarril[i] = y + alto / 2.0;
        preferencia[i] = 0.0;
        exito[i] = 0.0;
    }
}

void Anciano::setCentrosCarril(const double centros[3])
{
    for (int i = 0; i < 3; ++i)
        centroCarril[i] = centros[i];

    // Empieza alineado con el carril central.
    carrilObjetivo = 1;
    yObjetivo = centroCarril[1] - alturaSobreCarril - alto / 2.0;
    y = yObjetivo;
}

void Anciano::percibir(int carrilDelJugador, double dt)
{
    if (carrilDelJugador < 0 || carrilDelJugador > 2)
        return;

    carrilJugador = carrilDelJugador;
    if (dt > 0.0)
        preferencia[carrilDelJugador] += dt;
}

double Anciano::puntuacionCarril(int carril) const
{
    const double sumaPref = preferencia[0] + preferencia[1] + preferencia[2];
    const double sumaExito = exito[0] + exito[1] + exito[2];

    double p = carril == carrilJugador ? 1.0 : 0.0;           // dónde está ahora
    if (sumaPref > 0.0)
        p += 0.8 * preferencia[carril] / sumaPref;            // dónde suele estar
    if (sumaExito > 0.0)
        p += 0.6 * exito[carril] / sumaExito;                 // dónde acertó antes
    return p;
}

void Anciano::decidirCarril()
{
    int mejor = carrilJugador;
    double mejorPuntos = puntuacionCarril(mejor);

    for (int i = 0; i < 3; ++i) {
        const double puntos = puntuacionCarril(i);
        if (puntos > mejorPuntos) {
            mejorPuntos = puntos;
            mejor = i;
        }
    }

    carrilObjetivo = mejor;
    yObjetivo = centroCarril[mejor] - alturaSobreCarril - alto / 2.0;
    decisionTomada = true;
}

void Anciano::actualizar(double dt)
{
    if (dt <= 0.0)
        return;

    temporizador += dt;
    if (lanzamientoActivo) {
        tiempoDesdeLanzamiento += dt;
        if (tiempoDesdeLanzamiento >= 0.72) lanzamientoActivo = false;
    }

    // Olvido gradual: lo reciente pesa más que lo antiguo.
    const double olvido = std::exp(-0.05 * dt);
    for (int i = 0; i < 3; ++i)
        preferencia[i] *= olvido;

    // Razona con la información más reciente poco antes de lanzar.
    if (!decisionTomada && temporizador >= 0.55 * tiempoEntreLanzamiento)
        decidirCarril();

    // Acción: se desplaza verticalmente hacia el carril elegido.
    const double diferencia = yObjetivo - y;
    const double paso = velocidadVertical * dt;
    if (std::abs(diferencia) <= paso)
        y = yObjetivo;
    else
        y += diferencia > 0.0 ? paso : -paso;
}

bool Anciano::estaAlineado() const
{
    return std::abs(yObjetivo - y) < 6.0;
}

bool Anciano::listoParaLanzar() const
{
    return temporizador >= tiempoEntreLanzamiento && estaAlineado();
}

Proyectil *Anciano::lanzarBotella(double objetivoX, double objetivoY)
{
    if (!listoParaLanzar())
        return nullptr;

    temporizador = 0.0;
    decisionTomada = false;
    lanzamientoActivo = true;
    tiempoDesdeLanzamiento = 0.0;

    const double anchoBotella = 24.0;
    const double altoBotella = 30.0;

    // La botella sale de la mano del Ángel (lado izquierdo, hacia el jugador).
    const double xInicial = x - anchoBotella / 2.0;
    const double yInicial = y + alto * 0.4 - altoBotella / 2.0;

    // Se apunta con el centro de la botella al centro del objetivo. La parábola la
    // resuelve el propio Proyectil; el Ángel solo guarda el ángulo y la rapidez elegidos.
    Proyectil::calcularLanzamiento(
        xInicial, yInicial,
        objetivoX - anchoBotella / 2.0, objetivoY - altoBotella / 2.0,
        tiempoVuelo, velocidadInicial, angulo);

    Proyectil *botella = new Proyectil(xInicial, yInicial, anchoBotella, altoBotella,
                                       velocidadInicial, angulo, true, tiempoVuelo + 0.6);
    botella->setCarrilObjetivo(carrilObjetivo);
    return botella;
}

void Anciano::registrarResultado(int carril, bool impacto)
{
    if (carril < 0 || carril > 2)
        return;

    if (impacto)
        exito[carril] += 1.0;
    else
        exito[carril] *= 0.8;
}

void Anciano::setTiempoEntreLanzamiento(double segundos)
{
    tiempoEntreLanzamiento = std::max(2.0, segundos);
}

void Anciano::setTiempoVuelo(double segundos)
{
    tiempoVuelo = std::clamp(segundos, 0.8, 1.4);
}

int Anciano::getCarrilObjetivo() const { return carrilObjetivo; }
bool Anciano::getLanzamientoActivo() const { return lanzamientoActivo; }
double Anciano::getTiempoDesdeLanzamiento() const { return tiempoDesdeLanzamiento; }
