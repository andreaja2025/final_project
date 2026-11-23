#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsTextItem>
#include <QGraphicsRectItem>
#include <QGraphicsLineItem>
#include <QGraphicsPolygonItem>
#include <QPointF>
#include <QHash>
#include <QList>
#include <QPixmap>
#include <QTimer>
#include <QElapsedTimer>
#include "Juego.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class GestorAudio;
class QKeyEvent;
class QResizeEvent;
class QShowEvent;
class QEvent;

// Capa visual del Nivel 1. Solo dibuja el estado de Juego/Nivel (no contiene
// reglas del juego): escenario con scroll, sprites, HUD y pantallas de resultado.
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void actualizarJuego(); // Un "tick" del temporizador (~60 FPS).

private:
    Ui::MainWindow *ui;
    QGraphicsScene *scene;
    QTimer *timer;
    QElapsedTimer reloj;
    Juego juego;
    GestorAudio *audio;   // música y efectos (reacciona a los eventos del nivel)

    // Recursos ya escalados al tamaño con el que se dibujan.
    QPixmap pxTerminator, pxTerminatorAcelerando, pxTerminatorFrenando,
        pxTerminatorGameOver, pxTerminatorPerdioVida, pxTerminatorRapidin;
    QPixmap pxAnciano, pxAnciano2, pxAnciano3, pxAncianoPreparando,
        pxAncianoLanzando, pxAncianoDespues;
    QPixmap pxBotella, pxHueco, pxCaja, pxCono, pxLlanta, pxPremio, pxCorazon, pxPalmera;

    // Escenario.
    QGraphicsPixmapItem *fondoA;
    QGraphicsPixmapItem *fondoB;
    QGraphicsPixmapItem *carreteraItem;
    QList<QGraphicsItem*> palmeras;
    QList<QGraphicsLineItem*> lineasVelocidad;
    double anchoFondoCompuesto;

    // Personajes.
    QGraphicsItem *jugadorItem;
    QGraphicsItem *ancianoItem;
    QHash<const Entidades*, QGraphicsItem*> itemsEntidades;

    // HUD.
    QGraphicsRectItem *panelHud;
    QGraphicsRectItem *barraFondo;
    QGraphicsRectItem *barraRelleno;
    QList<QGraphicsItem*> corazones;
    QGraphicsTextItem *textoTiempo;
    QGraphicsTextItem *textoEstado;
    QGraphicsTextItem *textoRapidin;
    QGraphicsTextItem *textoCuenta;
    QGraphicsTextItem *textoAyuda;

    // Pantalla de resultado.
    QGraphicsRectItem *overlay;
    QGraphicsRectItem *panelResultado;
    QGraphicsTextItem *textoResultado;
    QGraphicsTextItem *textoMotivo;
    QGraphicsTextItem *textoDetalle;
    QGraphicsTextItem *textoReiniciar;

    // Animación.
    double scrollRuta;
    double tiempoAnimacion;
    double tiempoParpadeo;
    int vidasAnteriores;
    int estadoAnterior;
    double tiempoImpacto;
    bool efectoChoqueActivo;
    struct EstrellaVisual { QGraphicsPolygonItem *item; QPointF velocidad; double vida; };
    QList<EstrellaVisual> estrellasColision;

    void cargarRecursos();
    void construirEscenario();
    void construirHud();
    void construirResultado();
    void reiniciarPartida();

    QPixmap cargarEscalado(const QString &ruta, int ancho, int alto) const;
    QPixmap recortarTransparencia(const QPixmap &origen) const;
    QGraphicsItem *crearSprite(const QPixmap &pixmap, double ancho, double alto,
                               const QColor &colorRespaldo);
    QGraphicsTextItem *crearTexto(int tamano, const QColor &color, bool sombra = false);
    void centrarTexto(QGraphicsTextItem *texto, double centroX, double y);

    void actualizarEscenario(double dt);
    void sincronizarJugador();
    void sincronizarAnciano();
    void sincronizarEntidades();
    void sincronizarHud();
    void sincronizarResultado();
    void crearEfectoColision(bool fatal);
    void actualizarEfectosColision(double dt);
    void ajustarVista();
    QPixmap cargarSpriteRecortado(const QString &ruta, int ancho, int alto) const;
};

#endif