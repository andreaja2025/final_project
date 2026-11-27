#ifndef PERSONAJEPRINCIPAL_H
#define PERSONAJEPRINCIPAL_H

#include "Entidades.h"

enum EstadoPersonaje { NORMAL, RAPIDIN, SIN_CONTROL, AGRESIVO };

// Terminator, el protagonista (conduce la moto en el Nivel 1).
class PersonajePrincipal : public Entidades
{
private:
    unsigned short vida;
    unsigned short vidaMaxima;
    unsigned short velocidad;        // Velocidad de cambio de carril (px/s).
    unsigned short carrilActual;

    EstadoPersonaje estado;
    double tiempoEstado;
    double yCarril[3];
    double velocidadYObjetivo;
    double vxImpacto, vyImpacto;
    double tiempoRetroceso;
    bool enRetroceso;

    void iniciarRetroceso(double velX, double velY, double duracion);

public:
    PersonajePrincipal();        // Se crea una sola vez (dentro del Nivel); reiniciar() lo deja listo.

    // Deja al personaje como nuevo: vida completa, carril central, sin efectos.
    void reiniciar(double x, const double yCarriles[3], double ancho, double alto);

    void actualizar(double dt) override;
    void CambiarDeCarril(int direccion);
    void activarRapidin(double duracion);
    void perderControl(double duracion = 1.5);
    void recibirDanio();
    void morir();

    // Colisión inelástica contra un objeto fijo: calcula la velocidad resultante
    // (componente normal con coeficiente de restitución) e inicia el retroceso.
    void recibirImpacto(double normalX, double normalY,
                        double velObstaculoX, double velObstaculoY,
                        double restitucion, double duracion);
    bool estaEnRetroceso() const;

    unsigned short getVida() const;
    unsigned short getVidaMaxima() const;
    int getCarril() const;
    EstadoPersonaje getEstado() const;
    double getTiempoEstado() const;
    double getYCarril(int carril) const;
};

#endif // PERSONAJEPRINCIPAL_H
