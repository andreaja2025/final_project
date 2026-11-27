#include "Nivel.h"

#include <cmath>
#include <algorithm>

namespace {
// Escena de 1280 x 720: paisaje arriba y carretera de 3 carriles de 120 px.
constexpr double X_JUGADOR = 100.0;
constexpr double ANCHO_JUGADOR = 110.0;
constexpr double ALTO_JUGADOR = 56.0;
constexpr double X_ANCIANO = 1060.0;
constexpr double X_APARICION = 1350.0;       // Fuera de pantalla, a la derecha.
constexpr double FACTOR_RAPIDIN_SCROLL = 1.6; // Velocidad del escenario con el premio.
constexpr double FACTOR_RAPIDIN_TIEMPO = 1.5; // El trayecto "avanza" más rápido.
constexpr double SEPARACION_MINIMA = 300.0;   // Entre un obstáculo y cualquier objeto nuevo.

// Devuelve la primera casilla inactiva del arreglo, o nullptr si todas están en uso.
template <class T>
T *buscarLibre(T arreglo[], int cantidad)
{
    for (int i = 0; i < cantidad; ++i)
        if (!arreglo[i].estaActiva())
            return &arreglo[i];
    return nullptr;
}
}

Nivel::Nivel()
    : tiempo(0.0), estado(PREPARACION),
    motivoDerrota(SIN_DERROTA),
    tiempoPreparacion(3.0), duracionPreparacion(3.0),
    duracionNivel(60.0), velocidadBase(300.0), velocidadMaxima(420.0),
    distanciaRecorrida(0.0), colisionFatalEnCurso(false), tiempoRestanteImpacto(0.0),
    distanciaObstaculo(0.0), separacionObstaculo(500.0),
    distanciaHueco(0.0), separacionHueco(1400.0),
    distanciaPremio(0.0), separacionPremio(2000.0),
    generador(std::random_device{}()),
    numEventos(0), segundoCuentaAnterior(0), rapidinAnterior(false)
{
    centroCarril[0] = 390.0;
    centroCarril[1] = 510.0;
    centroCarril[2] = 630.0;
}

Nivel::~Nivel()
{
    // Nada que liberar: todo vive en arreglos del propio Nivel.
}

// Deja el nivel en un estado completamente limpio (se usa también para reiniciar).
void Nivel::cargar()
{
    for (int i = 0; i < MAX_ESTATICOS; ++i)
        estaticos[i].desactivar();
    for (int i = 0; i < MAX_ARMAS; ++i)
        armas[i].liberar();

    tiempo = 0.0;
    tiempoPreparacion = duracionPreparacion;
    distanciaRecorrida = 0.0;
    colisionFatalEnCurso = false;
    tiempoRestanteImpacto = 0.0;
    distanciaObstaculo = 0.0;
    distanciaHueco = 0.0;
    distanciaPremio = 0.0;
    separacionObstaculo = 500.0;
    separacionHueco = 1400.0;
    separacionPremio = 2000.0;
    motivoDerrota = SIN_DERROTA;
    estado = PREPARACION;
    numEventos = 0;
    segundoCuentaAnterior = 0;
    rapidinAnterior = false;

    double yTop[3];
    for (int i = 0; i < 3; ++i)
        yTop[i] = centroCarril[i] - ALTO_JUGADOR / 2.0;

    jugador.reiniciar(X_JUGADOR, yTop, ANCHO_JUGADOR, ALTO_JUGADOR);
    anciano.reiniciar(X_ANCIANO, 0.0, 90.0, 100.0, 3.5);
    anciano.setCentrosCarril(centroCarril);
    registrarEvento(EVENTO_NIVEL_INICIADO);
}

double Nivel::progreso() const
{
    return duracionNivel > 0.0 ? std::fmin(1.0, tiempo / duracionNivel) : 1.0;
}

double Nivel::velocidadActual() const
{
    const double base = velocidadBase + (velocidadMaxima - velocidadBase) * progreso();
    const bool rapidin = jugador.getEstado() == RAPIDIN;
    return rapidin ? base * FACTOR_RAPIDIN_SCROLL : base;
}

void Nivel::registrarEvento(EventoJuego evento)
{
    if (numEventos < MAX_EVENTOS)
        eventos[numEventos++] = evento;
}

int Nivel::getNumEventos() const { return numEventos; }

EventoJuego Nivel::getEvento(int indice) const { return eventos[indice]; }

void Nivel::limpiarEventos() { numEventos = 0; }

// Envoltorio: ejecuta la actualización y avisa de los cambios de estado del nivel.
void Nivel::actualizar(double dt)
{
    const EstadoNivel antes = estado;
    procesarActualizacion(dt);

    if (antes == estado)
        return;
    if (antes == PREPARACION && estado == JUGANDO)
        registrarEvento(EVENTO_YA);
    else if (estado == COMPLETADO)
        registrarEvento(EVENTO_NIVEL_COMPLETADO);
    else if (estado == PERDIDO)
        registrarEvento(EVENTO_NIVEL_PERDIDO);
}

void Nivel::procesarActualizacion(double dt)
{
    if (dt <= 0.0 || estado == COMPLETADO || estado == PERDIDO)
        return;

    if (estado == PREPARACION) {
        // Cuenta regresiva: se puede cambiar de carril, pero no corre el
        // tiempo de supervivencia, no se generan objetos y no hay colisiones.
        jugador.actualizar(dt);
        tiempoPreparacion -= dt;
        if (tiempoPreparacion <= 0.0) {
            tiempoPreparacion = 0.0;
            estado = JUGANDO;
        } else {
            // Un pitido por cada número de la cuenta regresiva (3, 2, 1).
            const int segundo = std::max(1, int(std::ceil(tiempoPreparacion)));
            if (segundo != segundoCuentaAnterior) {
                segundoCuentaAnterior = segundo;
                registrarEvento(EVENTO_CUENTA_REGRESIVA);
            }
        }
        return;
    }

    actualizarNivel1(dt);
}

void Nivel::actualizarNivel1(double dt)
{
    // Durante el impacto fatal se congela el mundo y solo avanza el retroceso
    // físico de Terminator. La derrota se declara al terminar la animación.
    if (colisionFatalEnCurso) {
        jugador.actualizar(dt);
        tiempoRestanteImpacto -= dt;
        if (tiempoRestanteImpacto <= 0.0) {
            colisionFatalEnCurso = false;
            jugador.morir();
            motivoDerrota = OBSTACULO_ESTATICO;
            estado = PERDIDO;
        }
        return;
    }

    if (!jugador.estaActiva()) {
        estado = PERDIDO;
        return;
    }

    jugador.actualizar(dt);

    const bool rapidinAhora = jugador.getEstado() == RAPIDIN;
    if (rapidinAnterior && !rapidinAhora)
        registrarEvento(EVENTO_RAPIDIN_FIN);
    rapidinAnterior = rapidinAhora;

    const double velocidad = velocidadActual();
    const bool rapidin = jugador.getEstado() == RAPIDIN;

    distanciaRecorrida += velocidad * dt;
    tiempo += dt * (rapidin ? FACTOR_RAPIDIN_TIEMPO : 1.0);

    // Dificultad creciente: el Ángel lanza más seguido y con arcos más rápidos.
    anciano.setTiempoEntreLanzamiento(3.5 - 0.9 * progreso());
    anciano.setTiempoVuelo(1.25 - 0.30 * progreso());
    anciano.percibir(jugador.getCarril(), dt);
    anciano.actualizar(dt);

    distanciaObstaculo += velocidad * dt;
    distanciaHueco += velocidad * dt;
    distanciaPremio += velocidad * dt;

    if (distanciaObstaculo >= separacionObstaculo)
        generarObstaculo();
    if (distanciaHueco >= separacionHueco)
        generarHueco();
    if (distanciaPremio >= separacionPremio)
        generarPremio();

    if (anciano.listoParaLanzar())
        lanzarBotella();

    // Lo que viaja con la carretera recibe su velocidad; las botellas vuelan solas (parábola).
    for (int i = 0; i < MAX_ESTATICOS; ++i) {
        ObjetosEstaticos &objeto = estaticos[i];
        if (!objeto.estaActiva())
            continue;
        objeto.setVelocidad(-velocidad, 0.0);
        objeto.actualizar(dt);
    }
    for (int i = 0; i < MAX_ARMAS; ++i) {
        ObjetosArmas &arma = armas[i];
        if (!arma.estaActiva())
            continue;
        if (!arma.esBotella())
            arma.setVelocidad(-velocidad, 0.0);
        arma.actualizar(dt);
    }

    verificarColisiones();
    revisarBotellas();

    // Si comenzó el retroceso fatal, no se continúa avanzando el mundo.
    if (colisionFatalEnCurso)
        return;

    // La derrota tiene prioridad sobre la victoria en el mismo ciclo.
    if (estado == JUGANDO && tiempo >= duracionNivel) {
        tiempo = duracionNivel;
        estado = COMPLETADO;
    }
}

void Nivel::entradas(int tecla)
{
    if (estado != PREPARACION && estado != JUGANDO)
        return;

    const int carrilAntes = jugador.getCarril();

    if (tecla == 'W' || tecla == 'w')
        jugador.CambiarDeCarril(-1);   // Carril superior.
    else if (tecla == 'S' || tecla == 's')
        jugador.CambiarDeCarril(1);    // Carril inferior.

    if (jugador.getCarril() != carrilAntes)
        registrarEvento(EVENTO_CAMBIO_CARRIL);
}

int Nivel::numeroAleatorio(int minimo, int maximo)
{
    return std::uniform_int_distribution<int>(minimo, maximo)(generador);
}

int Nivel::carrilDe(const Entidades *e) const
{
    int mejor = 0;
    double mejorDistancia = std::abs(e->getCentroY() - centroCarril[0]);
    for (int i = 1; i < 3; ++i) {
        const double d = std::abs(e->getCentroY() - centroCarril[i]);
        if (d < mejorDistancia) {
            mejorDistancia = d;
            mejor = i;
        }
    }
    return mejor;
}

// Elige un carril donde no haya aparecido hace poco otro objeto.
// Devuelve -1 si no se puede generar nada en este momento.
int Nivel::elegirCarrilLibre(bool exigirSinObstaculoCerca)
{
    bool ocupado[3] = {false, false, false};

    for (int i = 0; i < MAX_ESTATICOS; ++i) {
        const ObjetosEstaticos &objeto = estaticos[i];
        if (!objeto.estaActiva())
            continue;
        if (objeto.getX() < X_APARICION - SEPARACION_MINIMA)
            continue;                                   // Ya está lejos del punto de aparición.

        ocupado[carrilDe(&objeto)] = true;
        if (exigirSinObstaculoCerca && !objeto.esPremio())
            return -1;                                  // Nunca dos obstáculos fatales juntos.
    }

    for (int i = 0; i < MAX_ARMAS; ++i) {
        const ObjetosArmas &arma = armas[i];
        if (!arma.estaActiva() || arma.esBotella())
            continue;
        if (arma.getX() < X_APARICION - SEPARACION_MINIMA)
            continue;

        ocupado[carrilDe(&arma)] = true;
    }

    int libres[3];
    int cantidad = 0;
    for (int i = 0; i < 3; ++i)
        if (!ocupado[i])
            libres[cantidad++] = i;

    if (cantidad == 0)
        return -1;
    return libres[numeroAleatorio(0, cantidad - 1)];
}

void Nivel::generarObstaculo()
{
    const int carril = elegirCarrilLibre(true);
    if (carril < 0)
        return;                         // Se reintenta en el siguiente ciclo.

    ObjetosEstaticos *casilla = buscarLibre(estaticos, MAX_ESTATICOS);
    if (!casilla)
        return;                         // Sin casilla libre: también se reintenta.

    const auto tipo = static_cast<TipoObjetoEstatico>(numeroAleatorio(0, 2));   // CAJA, CONO o LLANTA
    double ancho = 54.0, alto = 50.0;
    if (tipo == CONO) { ancho = 36.0; alto = 56.0; }
    else if (tipo == LLANTA) { ancho = 52.0; alto = 52.0; }

    casilla->crearObstaculo(X_APARICION, centroCarril[carril] - alto / 2.0,
                            ancho, alto, tipo, carril);

    distanciaObstaculo = 0.0;
    separacionObstaculo = numeroAleatorio(760, 1000);
}

void Nivel::generarHueco()
{
    const int carril = elegirCarrilLibre(false);
    if (carril < 0)
        return;

    ObjetosArmas *casilla = buscarLibre(armas, MAX_ARMAS);
    if (!casilla)
        return;

    const double ancho = 76.0, alto = 28.0;
    casilla->crearHueco(X_APARICION, centroCarril[carril] - alto / 2.0, ancho, alto);

    distanciaHueco = 0.0;
    separacionHueco = numeroAleatorio(1900, 2500);
}

void Nivel::generarPremio()
{
    const int carril = elegirCarrilLibre(false);
    if (carril < 0)
        return;

    ObjetosEstaticos *casilla = buscarLibre(estaticos, MAX_ESTATICOS);
    if (!casilla)
        return;

    const double lado = 40.0;
    casilla->crearPremio(X_APARICION, centroCarril[carril] - lado / 2.0, lado, lado);

    distanciaPremio = 0.0;
    separacionPremio = numeroAleatorio(2200, 3000);
}

void Nivel::lanzarBotella()
{
    ObjetosArmas *casilla = buscarLibre(armas, MAX_ARMAS);
    if (!casilla)
        return;                         // Sin casilla libre: se reintenta en el siguiente ciclo.

    // El Ángel apunta al carril que él decidió (puede anticiparse al jugador).
    const double objetivoX = jugador.getCentroX();
    const double objetivoY = centroCarril[anciano.getCarrilObjetivo()];

    if (anciano.lanzarBotella(casilla, objetivoX, objetivoY))
        registrarEvento(EVENTO_LANZAMIENTO_BOTELLA);
}

void Nivel::verificarColisiones()
{
    // 1) Obstáculos fijos: derrota inmediata (no es pérdida de vida).
    for (int i = 0; i < MAX_ESTATICOS; ++i) {
        ObjetosEstaticos &objeto = estaticos[i];
        if (!objeto.estaActiva() || objeto.esPremio())
            continue;
        if (jugador.colisionaCon(&objeto)) {
            // El objeto aplica su efecto: choque inelástico que hace retroceder al personaje.
            objeto.colocarefect(&jugador);
            colisionFatalEnCurso = true;
            tiempoRestanteImpacto = ObjetosEstaticos::DURACION_IMPACTO;
            registrarEvento(EVENTO_CHOQUE_FATAL);
            motivoDerrota = OBSTACULO_ESTATICO;
            return;
        }
    }

    // 2) Premios.
    for (int i = 0; i < MAX_ESTATICOS; ++i) {
        ObjetosEstaticos &objeto = estaticos[i];
        if (!objeto.estaActiva() || !objeto.esPremio())
            continue;
        if (jugador.colisionaCon(&objeto)) {
            objeto.colocarefect(&jugador);
            registrarEvento(EVENTO_PREMIO);
        }
    }

    // 3) Botellas y huecos (quitan una vida).
    for (int i = 0; i < MAX_ARMAS; ++i) {
        ObjetosArmas &arma = armas[i];
        if (!arma.estaActiva() || !jugador.colisionaCon(&arma))
            continue;

        const bool botella = arma.esBotella();
        arma.colocarefect(&jugador);         // Se aplica una sola vez y se desactiva.
        if (botella) {
            anciano.registrarResultado(arma.getCarrilObjetivo(), true);
            registrarEvento(EVENTO_IMPACTO_BOTELLA);
        } else {
            registrarEvento(EVENTO_CAIDA_HUECO);
        }

        if (!jugador.estaActiva()) {
            motivoDerrota = VIDAS_AGOTADAS;
            estado = PERDIDO;
            return;
        }
    }
}

// Una botella que termina su trayectoria sin golpear es un fallo del Ángel. Se cuenta una
// sola vez, cuando su casilla queda libre (reemplaza a la antigua limpieza con delete).
void Nivel::revisarBotellas()
{
    for (int i = 0; i < MAX_ARMAS; ++i) {
        ObjetosArmas &arma = armas[i];
        if (arma.estaActiva() || !arma.resultadoPorRegistrar())
            continue;

        arma.resultadoRegistrado();
        if (!arma.huboImpacto()) {
            anciano.registrarResultado(arma.getCarrilObjetivo(), false);
            registrarEvento(EVENTO_BOTELLA_FALLADA);
        }
    }
}

bool Nivel::completado() const { return estado == COMPLETADO; }
bool Nivel::nocompletado() const { return estado == PERDIDO; }
EstadoNivel Nivel::getEstado() const { return estado; }
MotivoDerrota Nivel::getMotivoDerrota() const { return motivoDerrota; }
double Nivel::getTiempo() const { return tiempo; }
double Nivel::getTiempoPreparacion() const { return tiempoPreparacion; }
double Nivel::getDuracionNivel() const { return duracionNivel; }
double Nivel::getDistanciaRecorrida() const { return distanciaRecorrida; }
double Nivel::getVelocidadScroll() const { return colisionFatalEnCurso ? 0.0 : velocidadActual(); }
double Nivel::getCentroCarril(int carril) const
{
    return (carril >= 0 && carril <= 2) ? centroCarril[carril] : centroCarril[1];
}
PersonajePrincipal *Nivel::getJugador() { return &jugador; }
const PersonajePrincipal *Nivel::getJugador() const { return &jugador; }
Anciano *Nivel::getAnciano() { return &anciano; }
const Anciano *Nivel::getAnciano() const { return &anciano; }
const ObjetosEstaticos &Nivel::getEstatico(int indice) const { return estaticos[indice]; }
const ObjetosArmas &Nivel::getArma(int indice) const { return armas[indice]; }
