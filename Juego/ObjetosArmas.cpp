#include "ObjetosArmas.h"
#include "PersonajePrincipal.h"

ObjetosArmas::ObjetosArmas(double x, double y, double ancho, double alto)
    : Entidades(x, y, ancho, alto), efectoAplicado(false)
{
}

void ObjetosArmas::actualizar(double dt)
{
    if (activa && dt > 0.0)
        mover(dt);
}

void ObjetosArmas::colocarefect(PersonajePrincipal *jugador)
{
    if (!jugador || efectoAplicado || !activa)
        return;

    // El hueco quita una vida; el nivel solo termina si el personaje se queda sin vidas.
    jugador->recibirDanio();
    efectoAplicado = true;
    desactivar();
}

bool ObjetosArmas::efectoFueAplicado() const
{
    return efectoAplicado;
}
