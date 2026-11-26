#include "PersonajePrincipal.h"
#include <algorithm>
#include <cmath>

PersonajePrincipal::PersonajePrincipal(double x, const double yCarriles[3],
                                       double ancho, double alto)
    : Entidades(x, yCarriles[1], ancho, alto),
    vida(3), vidaMaxima(3), velocidad(400), carrilActual(1),
    estado(NORMAL), tiempoEstado(0.0), velocidadYObjetivo(yCarriles[1]),
    vxImpacto(0.0), vyImpacto(0.0), tiempoRetroceso(0.0), enRetroceso(false)
{
    for (int i = 0; i < 3; ++i)
        yCarril[i] = yCarriles[i];
}

void PersonajePrincipal::actualizar(double dt)
{
    if (dt <= 0.0)
        return;

    if (enRetroceso) {
        x += vxImpacto * dt;
        y += vyImpacto * dt;
        tiempoRetroceso -= dt;
        // El retroceso se amortigua gradualmente, como una pérdida de energía.
        const double amortiguacion = std::pow(0.12, dt / 0.42);
        vxImpacto *= amortiguacion;
        vyImpacto *= amortiguacion;
        if (tiempoRetroceso <= 0.0) {
            tiempoRetroceso = 0.0;
            vxImpacto = vyImpacto = 0.0;
            enRetroceso = false;
        }
        return;
    }

    if (tiempoEstado > 0.0) {
        tiempoEstado -= dt;
        if (tiempoEstado <= 0.0) {
            tiempoEstado = 0.0;
            estado = NORMAL;
        }
    }

    const double diferencia = velocidadYObjetivo - y;
    const double pasoMaximo = velocidad * dt;

    if (std::abs(diferencia) <= pasoMaximo)
        y = velocidadYObjetivo;
    else
        y += (diferencia > 0.0 ? pasoMaximo : -pasoMaximo);
}

void PersonajePrincipal::CambiarDeCarril(int direccion)
{
    if (estado == SIN_CONTROL || direccion == 0)
        return;

    const int nuevoCarril = int(carrilActual) + (direccion > 0 ? 1 : -1);
    if (nuevoCarril >= 0 && nuevoCarril <= 2) {
        carrilActual = static_cast<unsigned short>(nuevoCarril);
        velocidadYObjetivo = yCarril[carrilActual];
    }
}

void PersonajePrincipal::activarRapidin(double duracion)
{
    estado = RAPIDIN;
    tiempoEstado = duracion;
}

void PersonajePrincipal::perderControl(double duracion)
{
    estado = SIN_CONTROL;
    tiempoEstado = duracion;
}

void PersonajePrincipal::recibirDanio()
{
    if (vida > 0)
        --vida;

    if (vida == 0)
        morir();
}

void PersonajePrincipal::morir()
{
    vida = 0;
    enRetroceso = false;
    vxImpacto = vyImpacto = 0.0;
    desactivar();
}

void PersonajePrincipal::recibirImpacto(double normalX, double normalY,
                                        double velObstaculoX, double velObstaculoY,
                                        double restitucion, double duracion)
{
    // Por defecto la velocidad no cambia (normal inválida o cuerpos que se separan).
    double nuevaVx = vx;
    double nuevaVy = vy;

    const double longitud = std::sqrt(normalX * normalX + normalY * normalY);
    if (longitud > 1e-9) {
        const double nx = normalX / longitud;
        const double ny = normalY / longitud;
        restitucion = std::max(0.0, std::min(1.0, restitucion));

        // Velocidad relativa al obstáculo y su componente a lo largo de la normal.
        const double rvx = vx - velObstaculoX;
        const double rvy = vy - velObstaculoY;
        const double vn = rvx * nx + rvy * ny;

        if (vn < 0.0) {
            const double factor = (1.0 + restitucion) * vn;
            nuevaVx = (rvx - factor * nx) + velObstaculoX;
            nuevaVy = (rvy - factor * ny) + velObstaculoY;
        }
    }

    iniciarRetroceso(nuevaVx, nuevaVy, duracion);
}

void PersonajePrincipal::iniciarRetroceso(double velX, double velY, double duracion)
{
    enRetroceso = true;
    vxImpacto = velX;
    vyImpacto = velY;
    tiempoRetroceso = duracion;
}

bool PersonajePrincipal::estaEnRetroceso() const { return enRetroceso; }

unsigned short PersonajePrincipal::getVida() const { return vida; }
unsigned short PersonajePrincipal::getVidaMaxima() const { return vidaMaxima; }
int PersonajePrincipal::getCarril() const { return carrilActual; }
EstadoPersonaje PersonajePrincipal::getEstado() const { return estado; }
double PersonajePrincipal::getTiempoEstado() const { return tiempoEstado; }

double PersonajePrincipal::getYCarril(int carril) const
{
    return (carril >= 0 && carril <= 2) ? yCarril[carril] : y;
}
