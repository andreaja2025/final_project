#ifndef PREMIO_H
#define PREMIO_H

#include "Entidades.h"

class PersonajePrincipal;

// Premio de la carretera: al recogerlo tiene un efecto sobre el personaje
// (hoy, convertirlo en conductor "rapidín" durante duracionEfect segundos).
class Premio : public Entidades
{
private:
    bool premioRapidin;
    double duracionEfect;
    bool recogido;

public:
    Premio(double x, double y, double ancho, double alto,
           bool premioRapidin = true, double duracionEfect = 4.5);

    void actualizar(double dt) override;
    void colocarefect(PersonajePrincipal *jugador);

    bool fueRecogido() const;
};

#endif // PREMIO_H
