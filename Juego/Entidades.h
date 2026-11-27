#ifndef ENTIDADES_H
#define ENTIDADES_H

// Clase base de todas las entidades del juego. Las coordenadas (x, y)
// corresponden a la esquina superior izquierda de la caja de colisión
// (en Qt el eje Y crece hacia abajo).
//
// Memoria: las entidades NO se crean ni se destruyen durante la partida. El Nivel
// guarda arreglos de tamaño fijo y cada casilla se recicla: "activa == false" significa
// casilla libre.
class Entidades
{
protected:
    double x;
    double y;
    double ancho;
    double alto;
    double vx;
    double vy;
    bool activa;

public:
    Entidades(double x, double y, double ancho, double alto);
    virtual ~Entidades() = default;

    virtual void actualizar(double dt) = 0;

    // Avanza con la velocidad (vx, vy). Si la entidad sale por completo por la izquierda
    // de la pantalla se desactiva sola: su casilla queda libre para volver a usarse.
    void mover(double dt);

    bool colisionaCon(const Entidades *otra) const;
    double getX() const;
    double getY() const;
    double getAncho() const;
    double getAlto() const;
    double getCentroX() const;
    double getCentroY() const;
    double getVx() const;
    double getVy() const;
    bool estaActiva() const;
    void desactivar();
    void setVelocidad(double nuevoVx, double nuevoVy);
};

#endif // ENTIDADES_H
