#ifndef OBJETOSESTATICOS_H
#define OBJETOSESTATICOS_H

#include "Entidades.h"

class PersonajePrincipal;

enum TipoObjetoEstatico {
    CAJA,
    CONO,
    LLANTA
};

// Objetos que van apareciendo en la carretera (caja, cono, llanta).
// Se mueven con la carretera (vx, vy los fija el nivel con la velocidad de scroll).
// El choque con cualquiera de ellos termina el nivel.
class ObjetosEstaticos : public Entidades
{
private:
    TipoObjetoEstatico tipo;
    int carril;

public:
    static constexpr double RESTITUCION = 0.65;
    static constexpr double DURACION_IMPACTO = 1.2;   // Segundos de retroceso antes de perder.

    ObjetosEstaticos(double x, double y, double ancho, double alto,
                     TipoObjetoEstatico tipo, int carril);

    void actualizar(double dt) override;
    void colocarefect(PersonajePrincipal *jugador);

    TipoObjetoEstatico getTipo() const;
    int getCarril() const;
};

#endif // OBJETOSESTATICOS_H
