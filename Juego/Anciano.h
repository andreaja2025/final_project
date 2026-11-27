#ifndef ANCIANO_H
#define ANCIANO_H

#include "Entidades.h"

class ObjetosArmas;

// Ángel del Nivel 1 (un anciano con alas enormes agente autónomo sencillo):
//   Percepción:  detecta el carril de Terminator (percibir).
//   Razonamiento: decide a qué carril alinearse/apuntar (decidirCarril).
//   Acción:      se desplaza verticalmente y lanza botellas parabólicas.
//   Aprendizaje: recuerda los carriles preferidos de Terminator y el éxito
//                de sus lanzamientos (registrarResultado).
class Anciano : public Entidades
{
private:
    double tiempoEntreLanzamiento;
    double temporizador;
    double angulo;               // Ángulo de la última botella lanzada (rad, desde +X, Y hacia abajo).
    double velocidadInicial;     // Rapidez inicial de la última botella lanzada (px/s).
    double tiempoVuelo;
    double velocidadVertical;
    double alturaSobreCarril;

    double centroCarril[3];      // Y central de cada carril.
    int carrilJugador;           // Último carril percibido.
    int carrilObjetivo;          // Carril al que apunta / se alinea.
    double yObjetivo;            // Posición Y (esquina sup.) a la que se mueve.
    bool decisionTomada;
    double tiempoDesdeLanzamiento;
    bool lanzamientoActivo;

    double preferencia[3];       // Tiempo (con olvido) que Terminator pasa en cada carril.
    double exito[3];             // Aciertos recordados por carril.

    double puntuacionCarril(int carril) const;

public:
    Anciano();                   // Se crea una sola vez (dentro del Nivel); reiniciar() lo deja listo.

    // Deja al Ángel como nuevo: sin memoria de lo que "aprendió" en la partida anterior.
    void reiniciar(double x, double y, double ancho, double alto,
                   double tiempoEntreLanzamiento);

    void actualizar(double dt) override;

    void setCentrosCarril(const double centros[3]);
    void percibir(int carrilDelJugador, double dt);
    void decidirCarril();

    bool estaAlineado() const;
    bool listoParaLanzar() const;
    // Configura 'botella' (una casilla libre del Nivel) como una botella lanzada hacia el
    // objetivo. Devuelve false si todavía no es el momento de lanzar.
    bool lanzarBotella(ObjetosArmas *botella, double objetivoX, double objetivoY);
    void registrarResultado(int carril, bool impacto);

    void setTiempoEntreLanzamiento(double segundos);
    void setTiempoVuelo(double segundos);
    int getCarrilObjetivo() const;
    bool getLanzamientoActivo() const;
    double getTiempoDesdeLanzamiento() const;
};

#endif
