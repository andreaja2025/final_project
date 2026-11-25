#include "Premio.h"
#include "PersonajePrincipal.h"

Premio::Premio(double x, double y, double ancho, double alto,
               bool premioRapidin, double duracionEfect)
    : Entidades(x, y, ancho, alto),
    premioRapidin(premioRapidin), duracionEfect(duracionEfect), recogido(false)
{
}

void Premio::actualizar(double dt)
{
    if (activa && dt > 0.0)
        mover(dt);
}

void Premio::colocarefect(PersonajePrincipal *jugador)
{
    if (!jugador || recogido || !activa)
        return;

    if (premioRapidin)
        jugador->activarRapidin(duracionEfect);

    recogido = true;
    desactivar();
}

bool Premio::fueRecogido() const
{
    return recogido;
}
