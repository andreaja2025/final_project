#include "ObjetosEstaticos.h"
#include "PersonajePrincipal.h"

#include <algorithm>

ObjetosEstaticos::ObjetosEstaticos()
    : Entidades(0.0, 0.0, 0.0, 0.0), tipo(CAJA), carril(0),
    premioRapidin(false), duracionEfect(0.0), recogido(false)
{
    activa = false;
}

void ObjetosEstaticos::crearObstaculo(double x, double y, double ancho, double alto,
                                      TipoObjetoEstatico tipo, int carril)
{
    this->x = x;
    this->y = y;
    this->ancho = ancho;
    this->alto = alto;
    vx = vy = 0.0;
    this->tipo = tipo;
    this->carril = carril;
    premioRapidin = false;
    duracionEfect = 0.0;
    recogido = false;
    activa = true;
}

void ObjetosEstaticos::crearPremio(double x, double y, double ancho, double alto,
                                   bool premioRapidin, double duracionEfect)
{
    this->x = x;
    this->y = y;
    this->ancho = ancho;
    this->alto = alto;
    vx = vy = 0.0;
    tipo = PREMIO;
    carril = -1;
    this->premioRapidin = premioRapidin;
    this->duracionEfect = duracionEfect;
    recogido = false;
    activa = true;
}

void ObjetosEstaticos::actualizar(double dt)
{
    if (activa && dt > 0.0)
        mover(dt);
}

// Cada tipo aplica su propio efecto sobre el personaje.
void ObjetosEstaticos::colocarefect(PersonajePrincipal *jugador)
{
    if (!jugador || !activa)
        return;

    if (tipo == PREMIO)
        aplicarPremio(jugador);
    else
        aplicarChoque(jugador);
}

// Efecto del premio: el personaje pasa a "rapidín" y el premio desaparece (casilla libre).
void ObjetosEstaticos::aplicarPremio(PersonajePrincipal *jugador)
{
    if (recogido)
        return;

    if (premioRapidin)
        jugador->activarRapidin(duracionEfect);

    recogido = true;
    desactivar();
}

// Efecto del obstáculo estatico: choque inelástico contra el personaje (lo hace retroceder).
void ObjetosEstaticos::aplicarChoque(PersonajePrincipal *jugador)
{
    // Determinar el lado del objeto que recibió el impacto.
    const double solapeIzq = jugador->getX() + jugador->getAncho() - x;
    const double solapeDer = x + ancho - jugador->getX();
    const double solapeArr = jugador->getY() + jugador->getAlto() - y;
    const double solapeAba = y + alto - jugador->getY();
    const double minimo = std::min({solapeIzq, solapeDer, solapeArr, solapeAba});

    double normalX = 0.0, normalY = 0.0;
    if (minimo == solapeIzq) normalX = -1.0;        // Golpe por la derecha del jugador.
    else if (minimo == solapeDer) normalX = 1.0;    // Golpe por la izquierda.
    else if (minimo == solapeArr) normalY = -1.0;   // Golpe desde arriba.
    else normalY = 1.0;                             // Golpe desde abajo.

    // El objeto se desplaza hacia la izquierda con el scroll de la carretera.
    jugador->recibirImpacto(normalX, normalY, vx, vy, RESTITUCION, DURACION_IMPACTO);
}

TipoObjetoEstatico ObjetosEstaticos::getTipo() const { return tipo; }
int ObjetosEstaticos::getCarril() const { return carril; }
bool ObjetosEstaticos::esPremio() const { return tipo == PREMIO; }
bool ObjetosEstaticos::fueRecogido() const { return recogido; }
