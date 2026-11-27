#ifndef NIVEL_H
#define NIVEL_H

#include <random>
#include "Entidades.h"
#include "EventoJuego.h"
#include "PersonajePrincipal.h"
#include "Anciano.h"
#include "ObjetosEstaticos.h"
#include "ObjetosArmas.h"

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

// Nivel activo ( el Nivel 1: moto de 3 carriles). Contiene (composición) las
// entidades de la partida y coordina las reglas: generación, colisiones, victoria/derrota.
//
// Memoria: todo lo que existe en la partida vive en arreglos de tamaño fijo dentro del Nivel.
// No hay new/delete mientras se juega en nivel 1: cuando algo aparece se busca una casilla inactiva y se
// reutiliza; cuando desaparece (choque, premio recogido, sale de pantalla) solo se desactiva.
// Los tamaños salen de las reglas de generación (separaciones mínimas y velocidad de scroll):
//   obstáculos: separación mínima 760 px y recorren ~1500 px  ->  2 a la vez
//   premios:    separación mínima 2200 px                     ->  1 a la vez
//   huecos:     separación mínima 1900 px                     ->  1 a la vez
//   botellas:   un lanzamiento cada 2 s como mínimo y vida <= 2 s -> 1 a la vez (2 por margen)
// Cada arreglo tiene margen de sobra; si aun así se llenara, la aparición se reintenta.
class Nivel
{
public:
    static constexpr int MAX_ESTATICOS = 6;   // obstáculos + premios
    static constexpr int MAX_ARMAS = 5;       // huecos + botellas
    static constexpr int MAX_EVENTOS = 16;    // eventos pendientes por ciclo

protected:
    PersonajePrincipal jugador;
    double tiempo;                 // Tiempo de supervivencia transcurrido (máx. duracionNivel).
    EstadoNivel estado;

private:
    MotivoDerrota motivoDerrota;

    Anciano anciano;
    ObjetosEstaticos estaticos[MAX_ESTATICOS];   // Cajas, conos, llantas y premios.
    ObjetosArmas armas[MAX_ARMAS];               // Huecos y botellas.

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
    EventoJuego eventos[MAX_EVENTOS];
    int numEventos;
    int segundoCuentaAnterior;
    bool rapidinAnterior;

    void registrarEvento(EventoJuego evento);
    void procesarActualizacion(double dt);

    int numeroAleatorio(int minimo, int maximo);
    int carrilDe(const Entidades *e) const;
    int elegirCarrilLibre(bool exigirSinObstaculoCerca);
    double velocidadActual() const;
    double progreso() const;

    void revisarBotellas();
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

    PersonajePrincipal *getJugador();
    const PersonajePrincipal *getJugador() const;
    Anciano *getAnciano();
    const Anciano *getAnciano() const;

    // Casillas de los arreglos (activas o libres). La capa visual crea un sprite por casilla.
    const ObjetosEstaticos &getEstatico(int indice) const;
    const ObjetosArmas &getArma(int indice) const;

    // Eventos ocurridos desde la última limpieza (0 .. getNumEventos()-1).
    int getNumEventos() const;
    EventoJuego getEvento(int indice) const;
    void limpiarEventos();
};

#endif // NIVEL_H
