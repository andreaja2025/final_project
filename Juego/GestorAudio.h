#ifndef GESTORAUDIO_H
#define GESTORAUDIO_H

#include <QElapsedTimer>
#include <QObject>
#include <QSoundEffect>

#include "EventoJuego.h"

// Capa de audio: traduce los eventos del juego (EventoJuego) en música y efectos de sonido.
// No conoce las reglas del nivel; solo reacciona a lo que Nivel informa.
//
// Interruptor: el audio se puede activar/desactivar con setActivo(). Hoy el juego lo crea
// siempre activo; si más adelante se agrega una opción se espera agregar la opción "silenciar"
class GestorAudio : public QObject
{
    Q_OBJECT

public:
    explicit GestorAudio(bool activo = true, QObject *parent = nullptr);

    void setActivo(bool activo);
    bool estaActivo() const;

    void procesarEvento(EventoJuego evento);

private:
    enum Sonido {
        MUSICA_NIVEL, MUSICA_RAPIDIN,
        FX_CARRIL, FX_CUENTA, FX_YA, FX_LANZAMIENTO, FX_IMPACTO_BOTELLA, FX_BOTELLA_FALLADA,
        FX_HUECO, FX_PREMIO, FX_RAPIDIN_FIN, FX_CHOQUE, FX_VICTORIA, FX_DERROTA,
        TOTAL_SONIDOS
    };

    enum class Musica { Ninguna, Nivel, Rapidin };

    QSoundEffect *sonidos[TOTAL_SONIDOS];
    bool pendiente[TOTAL_SONIDOS];
    qint64 pedidoEn[TOTAL_SONIDOS];
    QElapsedTimer reloj;
    bool activo;
    Musica musicaActual;               // pista que debe sonar (aunque el audio esté apagado)

    void cargar(Sonido id, const char *archivo, double volumen, bool enBucle);
    void reproducir(Sonido id);
    void detener(Sonido id);
    void alCambiarEstado(Sonido id);
    void cambiarMusica(Musica nueva);
    void aplicarMusica();
    static bool esMusica(Sonido id);
};

#endif // GESTORAUDIO_H
