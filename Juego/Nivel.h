#ifndef NIVEL_H
#define NIVEL_H

#include <random>
#include <vector>
#include "Entidades.h"
#include "EventoJuego.h"

class PersonajePrincipal;
class Anciano;

enum EstadoNivel {
    PREPARACION,
    JUGANDO,
    COMPLETADO,
    PERDIDO
};

enum MotivoDerrota {
    SIN_DERROTA,
    VIDAS_AGOTADAS,
    OBSTACULO_ESTATICO
};

// Nivel activo (hoy, el Nivel 1: moto de 3 carriles). Contiene (composición) las
// entidades de la partida y coordina las reglas: generación, colisiones, victoria/derrota.
class Nivel
{
protected:
    PersonajePrincipal *jugador;
    double tiempo;                 // Tiempo de supervivencia transcurrido (máx. duracionNivel).
    EstadoNivel estado;

private:
    MotivoDerrota motivoDerrota;

    Anciano *anciano;
    std::vector<Entidades*> entidades;   // Huecos, botellas, objetos estáticos y premios.

    double tiempoPreparacion;      // Cuenta regresiva restante.
    double duracionPreparacion;
    double duracionNivel;
    double centroCarril[3];        // Y central de cada carril.
    double velocidadBase;          // Velocidad de scroll al inicio (px/s).
    double velocidadMaxima;        // Velocidad de scroll al final (px/s).
    double distanciaRecorrida;
    bool colisionFatalEnCurso;
    double tiempoRestanteImpacto;

    // El generador de objetos trabaja por distancia recorrida (no por tiempo),
    // así la separación entre objetos es constante aunque aumente la velocidad.
    double distanciaObstaculo, separacionObstaculo;
    double distanciaHueco, separacionHueco;
    double distanciaPremio, separacionPremio;

    std::mt19937 generador;

    // Cola de eventos para la capa de presentación (sonido). Nivel solo informa qué
    // pasó; no sabe nada de audio ni de Qt.
    std::vector<EventoJuego> eventos;
    int segundoCuentaAnterior;
    bool rapidinAnterior;

    void registrarEvento(EventoJuego evento);
    void procesarActualizacion(double dt);

    int numeroAleatorio(int minimo, int maximo);
    int carrilDe(const Entidades *e) const;
    int elegirCarrilLibre(bool exigirSinObstaculoCerca);
    double velocidadActual() const;
    double progreso() const;

    void descartarEntidades();
    void limpiarInactivas();
    void generarObstaculo();
    void generarHueco();
    void generarPremio();
    void lanzarBotella();
    void verificarColisiones();
    void actualizarNivel1(double dt);

public:
    Nivel();
    virtual ~Nivel();

    Nivel(const Nivel&) = delete;
    Nivel& operator=(const Nivel&) = delete;

    virtual void cargar();
    virtual void actualizar(double dt);
    virtual void entradas(int tecla);
    virtual bool completado() const;
    virtual bool nocompletado() const;

    EstadoNivel getEstado() const;
    MotivoDerrota getMotivoDerrota() const;

    double getTiempo() const;
    double getTiempoPreparacion() const;
    double getDuracionNivel() const;
    double getDistanciaRecorrida() const;
    double getVelocidadScroll() const;
    double getCentroCarril(int carril) const;

    PersonajePrincipal *getJugador() const;
    Anciano *getAnciano() const;
    const std::vector<Entidades*>& getEntidades() const;

    // Devuelve los eventos ocurridos desde la última llamada y vacía la cola.
    std::vector<EventoJuego> tomarEventos();
};

#endif // NIVEL_H
