#include "ObjetosEstaticos.h"
#include "PersonajePrincipal.h"

#include <algorithm>

ObjetosEstaticos::ObjetosEstaticos(
    double x, double y, double ancho, double alto,
    TipoObjetoEstatico tipo, int carril)
    : Entidades(x, y, ancho, alto), tipo(tipo), carril(carril)
{
}

void ObjetosEstaticos::actualizar(double dt)
{
    if (activa && dt > 0.0)
        mover(dt);
}

// Efecto: choque inelástico contra el personaje (lo hace retroceder).
void ObjetosEstaticos::colocarefect(PersonajePrincipal *jugador)
{
    if (!jugador || !activa)
        return;

    // Determinar el lado del objeto que recibió el impacto.
    const double solapeIzq = jugador->getX() + jugador->getAncho() - x;
    const double solapeDer = x + ancho - jugador->getX();
    const double solapeArr = jugador->getY() + jugador->getAlto() - y;
    const double solapeAba = y + alto - jugador->getY();
    const double minimo = std::min({solapeIzq, solapeDer, solapeArr, solapeAba});

    double normalX = 0.0, normalY = 0.0;
    if (minimo == solapeIzq) normalX = -1.0;        // Golpe por la derecha del jugador.
    else if (minimo == solapeDer) normalX = 1.0;    // Golpe por la izquierda.
    else if (minimo == solapeArr) normalY = -1.0;   // Golpe desde arriba.
    else normalY = 1.0;                             // Golpe desde abajo.

    // El objeto se desplaza hacia la izquierda con el scroll de la carretera.
    jugador->recibirImpacto(normalX, normalY, vx, vy, RESTITUCION, DURACION_IMPACTO);
}

TipoObjetoEstatico ObjetosEstaticos::getTipo() const
{
    return tipo;
}

int ObjetosEstaticos::getCarril() const
{
    return carril;
}
