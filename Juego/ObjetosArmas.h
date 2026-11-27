#ifndef OBJETOSARMAS_H
#define OBJETOSARMAS_H

#include "Entidades.h"

class PersonajePrincipal;

enum TipoArma {
    HUECO,       // viaja con la carretera
    BOTELLA      // proyectil lanzado por el Ángel (movimiento parabólico)
};

// Objetos que le quitan una vida al personaje (no terminan el nivel por sí solos):
//   - HUECO: está en la carretera y se mueve con ella.
//   - BOTELLA: la lanza el Ángel. Todo el movimiento parabólico vive aquí:
//       calcularLanzamiento(): velocidad inicial y ángulo para llegar a un objetivo.
//       actualizar(): posición y velocidad de la trayectoria en cada instante.
// El Nivel guarda un arreglo fijo de estos objetos y reutiliza las casillas inactivas.
class ObjetosArmas : public Entidades
{
private:
    TipoArma tipo;
    bool efectoAplicado;         // Hueco: ya quitó la vida. Botella: ya golpeó al personaje.

    // Trayectoria parabólica (solo se usa cuando tipo == BOTELLA).
    double x0;
    double y0;
    double vx0;
    double vy0;
    double tiempoVida;
    double tiempoTranscurrido;
    int carrilObjetivo;          // Carril al que apuntó el Ángel (-1 = ninguno).
    bool resultadoPendiente;     // La botella aún no se ha contado como acierto o fallo.

public:
    static constexpr double GRAVEDAD = 980.0;   // px/s^2 (el eje Y crece hacia abajo).

    ObjetosArmas();              // Casilla libre (inactiva).

    // Reciclan la casilla: le dan valores nuevos y la activan.
    void crearHueco(double x, double y, double ancho, double alto);
    // El ángulo está en radianes, medido desde el eje +X de la escena (Y hacia abajo).
    void crearBotella(double x, double y, double ancho, double alto,
                      double velocidadInicial, double angulo,
                      double tiempoVida, int carrilObjetivo);

    // Deja la casilla libre sin que cuente como botella fallada (se usa al reiniciar).
    void liberar();

    void actualizar(double dt) override;
    void colocarefect(PersonajePrincipal *jugador);

    // Resuelve la parábola que une el punto inicial con el objetivo en 'tiempoVuelo' segundos.
    static void calcularLanzamiento(double xInicial, double yInicial,
                                    double xObjetivo, double yObjetivo,
                                    double tiempoVuelo,
                                    double &velocidadInicial, double &angulo);

    TipoArma getTipo() const;
    bool esBotella() const;
    bool efectoFueAplicado() const;
    bool huboImpacto() const;
    int getCarrilObjetivo() const;

    // Una botella que ya terminó se cuenta una sola vez (acierto o fallo del Ángel).
    bool resultadoPorRegistrar() const;
    void resultadoRegistrado();
};

#endif // OBJETOSARMAS_H
