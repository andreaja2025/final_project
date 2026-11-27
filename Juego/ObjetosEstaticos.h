#ifndef OBJETOSESTATICOS_H
#define OBJETOSESTATICOS_H

#include "Entidades.h"

class PersonajePrincipal;

enum TipoObjetoEstatico {
    CAJA,
    CONO,
    LLANTA,
    PREMIO      // el premio "rapidín" también es un objeto que viaja con la carretera
};

// Objetos que van apareciendo en la carretera y viajan con ella (vx, vy los fija el nivel
// con la velocidad de scroll):
//   - CAJA, CONO, LLANTA: obstáculos. El choque con cualquiera de ellos termina el nivel.
//   - PREMIO: al recogerlo tiene un efecto sobre el personaje ("rapidín").
// El Nivel guarda un arreglo fijo de estos objetos y reutiliza las casillas inactivas.
class ObjetosEstaticos : public Entidades
{
private:
    TipoObjetoEstatico tipo;
    int carril;                  // Carril del obstáculo (-1 para el premio).

    // Datos del premio (solo se usan cuando tipo == PREMIO).
    bool premioRapidin;
    double duracionEfect;
    bool recogido;

    void aplicarPremio(PersonajePrincipal *jugador);
    void aplicarChoque(PersonajePrincipal *jugador);

public:
    static constexpr double RESTITUCION = 0.65;
    static constexpr double DURACION_IMPACTO = 1.2;   // Segundos de retroceso antes de perder.
    static constexpr double DURACION_RAPIDIN = 4.5;   // Segundos que dura el premio.

    ObjetosEstaticos();          // Casilla libre (inactiva).

    // Reciclan la casilla: le dan valores nuevos y la activan.
    void crearObstaculo(double x, double y, double ancho, double alto,
                        TipoObjetoEstatico tipo, int carril);
    void crearPremio(double x, double y, double ancho, double alto,
                     bool premioRapidin = true, double duracionEfect = DURACION_RAPIDIN);

    void actualizar(double dt) override;
    void colocarefect(PersonajePrincipal *jugador);

    TipoObjetoEstatico getTipo() const;
    int getCarril() const;
    bool esPremio() const;
    bool fueRecogido() const;
};

#endif // OBJETOSESTATICOS_H
