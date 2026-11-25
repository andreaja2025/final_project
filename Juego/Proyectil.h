#ifndef PROYECTIL_H
#define PROYECTIL_H

#include "Entidades.h"

class PersonajePrincipal;

// Botella lanzada por el Ángel. Todo el movimiento parabólico vive aquí:
//  - calcularLanzamiento(): velocidad inicial y ángulo para llegar a un objetivo.
//  - actualizar(): posición y velocidad de la trayectoria en cada instante.
class Proyectil : public Entidades
{
private:
    bool botella;
    double x0;
    double y0;
    double vx0;
    double vy0;
    double tiempoVida;
    double tiempoTranscurrido;
    int carrilObjetivo;          // Carril al que apuntó el Ángel (-1 = ninguno).
    bool impacto;                // Ya golpeó al personaje.

public:
    static constexpr double GRAVEDAD = 980.0;   // px/s^2 (el eje Y crece hacia abajo).

    // El ángulo está en radianes, medido desde el eje +X de la escena (Y hacia abajo).
    Proyectil(double x, double y, double ancho, double alto,
              double velocidadInicial, double angulo,
              bool botella = true, double tiempoVida = 5.0);

    void actualizar(double dt) override;
    void colocarefect(PersonajePrincipal *jugador);

    // Resuelve la parábola que une el punto inicial con el objetivo en 'tiempoVuelo' segundos.
    static void calcularLanzamiento(double xInicial, double yInicial,
                                    double xObjetivo, double yObjetivo,
                                    double tiempoVuelo,
                                    double &velocidadInicial, double &angulo);

    bool esBotella() const;
    bool huboImpacto() const;
    void setCarrilObjetivo(int carril);
    int getCarrilObjetivo() const;
};

#endif // PROYECTIL_H
