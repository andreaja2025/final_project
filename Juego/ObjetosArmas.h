#ifndef OBJETOSARMAS_H
#define OBJETOSARMAS_H

#include "Entidades.h"

class PersonajePrincipal;

// Hueco de la carretera: no termina el nivel por sí solo, pero quita una vida
// al personaje (la botella es un Proyectil). Se mueve con la carretera.
class ObjetosArmas : public Entidades
{
private:
    bool efectoAplicado;

public:
    ObjetosArmas(double x, double y, double ancho, double alto);

    void actualizar(double dt) override;
    void colocarefect(PersonajePrincipal *jugador);

    bool efectoFueAplicado() const;
};

#endif // OBJETOSARMAS_H
