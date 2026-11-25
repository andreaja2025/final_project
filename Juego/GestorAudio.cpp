#include "GestorAudio.h"

#include <QDebug>
#include <QString>
#include <QUrl>

namespace {
constexpr double VOLUMEN_MUSICA = 0.35;     // la música queda por debajo de los efectos
constexpr double VOLUMEN_EFECTOS = 0.80;
// Si un efecto se pide mientras su archivo todavía carga, solo suena si el pedido es reciente.
constexpr qint64 VIGENCIA_PEDIDO_MS = 800;
}

GestorAudio::GestorAudio(bool activo, QObject *parent)
    : QObject(parent), activo(activo), musicaActual(Musica::Nivel)
{
    reloj.start();
    for (int i = 0; i < TOTAL_SONIDOS; ++i) {
        sonidos[i] = nullptr;
        pendiente[i] = false;
        pedidoEn[i] = 0;
    }

    // Los archivos están dentro del ejecutable (recursos.qrc, carpeta assets/audio).
    cargar(MUSICA_NIVEL,       "musica_nivel.wav",    VOLUMEN_MUSICA,  true);
    cargar(MUSICA_RAPIDIN,     "musica_rapidin.wav",  VOLUMEN_MUSICA,  true);
    cargar(FX_CARRIL,          "carril.wav",          VOLUMEN_EFECTOS * 0.6, false);
    cargar(FX_CUENTA,          "cuenta.wav",          VOLUMEN_EFECTOS, false);
    cargar(FX_YA,              "ya.wav",              VOLUMEN_EFECTOS, false);
    cargar(FX_LANZAMIENTO,     "lanzamiento.wav",     VOLUMEN_EFECTOS, false);
    cargar(FX_IMPACTO_BOTELLA, "impacto_botella.wav", VOLUMEN_EFECTOS, false);
    cargar(FX_BOTELLA_FALLADA, "botella_fallada.wav", VOLUMEN_EFECTOS * 0.7, false);
    cargar(FX_HUECO,           "hueco.wav",           VOLUMEN_EFECTOS, false);
    cargar(FX_PREMIO,          "premio.wav",          VOLUMEN_EFECTOS, false);
    cargar(FX_RAPIDIN_FIN,     "rapidin_fin.wav",     VOLUMEN_EFECTOS, false);
    cargar(FX_CHOQUE,          "choque.wav",          VOLUMEN_EFECTOS, false);
    cargar(FX_VICTORIA,        "victoria.wav",        VOLUMEN_EFECTOS, false);
    cargar(FX_DERROTA,         "derrota.wav",         VOLUMEN_EFECTOS, false);

    // La música arranca sola al abrir el juego: el jugador no tiene que hacer nada.
    aplicarMusica();
}

void GestorAudio::cargar(Sonido id, const char *archivo, double volumen, bool enBucle)
{
    QSoundEffect *efecto = new QSoundEffect(this);
    efecto->setSource(QUrl(QStringLiteral("qrc:/audio/") + QString::fromLatin1(archivo)));
    efecto->setVolume(volumen);
    if (enBucle)
        efecto->setLoopCount(QSoundEffect::Infinite);

    // Cuando el archivo termina de cargar, si alguien pidió sonar, suena en ese momento.
    connect(efecto, &QSoundEffect::statusChanged, this, [this, id]() { alCambiarEstado(id); });
    sonidos[id] = efecto;
}

bool GestorAudio::esMusica(Sonido id)
{
    return id == MUSICA_NIVEL || id == MUSICA_RAPIDIN;
}

void GestorAudio::alCambiarEstado(Sonido id)
{
    QSoundEffect *efecto = sonidos[id];

    if (efecto->status() == QSoundEffect::Error) {
        qWarning() << "No se pudo cargar el sonido" << efecto->source();
        pendiente[id] = false;
        return;
    }
    if (efecto->status() != QSoundEffect::Ready || !pendiente[id])
        return;

    pendiente[id] = false;
    if (!activo)
        return;

    if (esMusica(id)) {
        // La música solo suena si sigue siendo la pista que se pidió.
        const bool vigente = (id == MUSICA_NIVEL && musicaActual == Musica::Nivel)
                             || (id == MUSICA_RAPIDIN && musicaActual == Musica::Rapidin);
        if (vigente)
            efecto->play();
    } else if (reloj.elapsed() - pedidoEn[id] <= VIGENCIA_PEDIDO_MS) {
        efecto->play();
    }
}

void GestorAudio::reproducir(Sonido id)
{
    if (!activo)
        return;

    QSoundEffect *efecto = sonidos[id];
    if (efecto->status() == QSoundEffect::Ready) {
        efecto->play();
    } else if (efecto->status() == QSoundEffect::Loading) {
        pendiente[id] = true;
        pedidoEn[id] = reloj.elapsed();
    }
}

void GestorAudio::detener(Sonido id)
{
    pendiente[id] = false;
    sonidos[id]->stop();
}

void GestorAudio::cambiarMusica(Musica nueva)
{
    musicaActual = nueva;
    aplicarMusica();
}

// Deja sonando solo la pista que corresponde (o ninguna).
void GestorAudio::aplicarMusica()
{
    detener(MUSICA_NIVEL);
    detener(MUSICA_RAPIDIN);

    if (!activo)
        return;

    if (musicaActual == Musica::Nivel)
        reproducir(MUSICA_NIVEL);
    else if (musicaActual == Musica::Rapidin)
        reproducir(MUSICA_RAPIDIN);
}

void GestorAudio::setActivo(bool nuevoEstado)
{
    if (nuevoEstado == activo)
        return;

    activo = nuevoEstado;

    if (!activo) {
        for (int i = 0; i < TOTAL_SONIDOS; ++i)
            detener(static_cast<Sonido>(i));
    } else {
        aplicarMusica();   // vuelve la música que corresponde al momento actual
    }
}

bool GestorAudio::estaActivo() const
{
    return activo;
}

void GestorAudio::procesarEvento(EventoJuego evento)
{
    switch (evento) {
    case EVENTO_NIVEL_INICIADO:
        detener(FX_VICTORIA);
        detener(FX_DERROTA);
        detener(FX_CHOQUE);
        if (musicaActual != Musica::Nivel)      // al reiniciar vuelve la música del nivel
            cambiarMusica(Musica::Nivel);
        break;

    case EVENTO_CUENTA_REGRESIVA:
        reproducir(FX_CUENTA);
        break;
    case EVENTO_YA:
        reproducir(FX_YA);
        break;
    case EVENTO_CAMBIO_CARRIL:
        reproducir(FX_CARRIL);
        break;
    case EVENTO_LANZAMIENTO_BOTELLA:
        reproducir(FX_LANZAMIENTO);
        break;
    case EVENTO_IMPACTO_BOTELLA:
        reproducir(FX_IMPACTO_BOTELLA);
        break;
    case EVENTO_BOTELLA_FALLADA:
        reproducir(FX_BOTELLA_FALLADA);
        break;
    case EVENTO_CAIDA_HUECO:
        reproducir(FX_HUECO);
        break;

    case EVENTO_PREMIO:
        reproducir(FX_PREMIO);
        if (musicaActual != Musica::Rapidin)    // el rapidín tiene su propia música, más veloz
            cambiarMusica(Musica::Rapidin);
        break;
    case EVENTO_RAPIDIN_FIN:
        reproducir(FX_RAPIDIN_FIN);
        if (musicaActual == Musica::Rapidin)
            cambiarMusica(Musica::Nivel);
        break;

    case EVENTO_CHOQUE_FATAL:
        cambiarMusica(Musica::Ninguna);         // silencio dramático antes del choque
        reproducir(FX_CHOQUE);
        break;
    case EVENTO_NIVEL_COMPLETADO:
        cambiarMusica(Musica::Ninguna);
        reproducir(FX_VICTORIA);
        break;
    case EVENTO_NIVEL_PERDIDO:
        cambiarMusica(Musica::Ninguna);
        reproducir(FX_DERROTA);
        break;
    }
}
