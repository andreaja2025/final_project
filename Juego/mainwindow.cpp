#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "PersonajePrincipal.h"
#include "Anciano.h"
#include "ObjetosEstaticos.h"
#include "ObjetosArmas.h"
#include "Premio.h"
#include "Proyectil.h"
#include "GestorAudio.h"

#include <QBrush>
#include <QColor>
#include <QDebug>
#include <QEvent>
#include <QFont>
#include <QGraphicsDropShadowEffect>
#include <QImage>
#include <QKeyEvent>
#include <QPainter>
#include <QPen>
#include <QResizeEvent>
#include <QSet>
#include <QShowEvent>

#include <algorithm>
#include <cmath>

namespace {
constexpr double ANCHO_ESCENA = 1280.0;
constexpr double ALTO_ESCENA = 720.0;
constexpr double Y_CARRETERA = 330.0;          // Borde superior de la carretera.
constexpr double ALTO_CARRETERA = 360.0;       // 3 carriles x 120 px.
constexpr double ANCHO_TESELA = 256.0;         // Ancho del mosaico de carretera.
constexpr double ALTO_FONDO = 300.0;           // Paisaje (parte superior).
constexpr double PARALAJE_FONDO = 0.18;        // El paisaje se mueve más lento.
constexpr int NUM_PALMERAS = 5;
constexpr double SEPARACION_PALMERAS = 300.0;
constexpr int NUM_LINEAS = 9;

// Interruptor del sonido. Hoy siempre está activo; en el futuro puede venir de una opción
// del juego (menú, tecla, archivo de configuración) y se cambia con audio->setActivo(...).
constexpr bool AUDIO_ACTIVO = true;

constexpr double Z_FONDO = 0, Z_BERMA = 1, Z_CARRETERA = 2, Z_PALMERA = 3,
    Z_ENTIDAD = 10, Z_ANCIANO = 30, Z_BOTELLA = 31,
    Z_LINEAS = 40, Z_HUD = 60, Z_RESULTADO = 80;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow),
    scene(new QGraphicsScene(this)), timer(new QTimer(this)),
    fondoA(nullptr), fondoB(nullptr), carreteraItem(nullptr),
    anchoFondoCompuesto(0.0), jugadorItem(nullptr), ancianoItem(nullptr),
    panelHud(nullptr), barraFondo(nullptr), barraRelleno(nullptr),
    textoTiempo(nullptr), textoEstado(nullptr), textoRapidin(nullptr),
    textoCuenta(nullptr), textoAyuda(nullptr),
    overlay(nullptr), panelResultado(nullptr), textoResultado(nullptr),
    textoMotivo(nullptr), textoDetalle(nullptr), textoReiniciar(nullptr),
    scrollRuta(0.0), tiempoAnimacion(0.0), tiempoParpadeo(0.0), vidasAnteriores(3),
    estadoAnterior(PREPARACION), tiempoImpacto(0.0), efectoChoqueActivo(false)
{
    ui->setupUi(this);
    setWindowTitle("Operación Macondo | Nivel 1");

    // La música empieza sola apenas se crea el gestor de audio.
    audio = new GestorAudio(AUDIO_ACTIVO, this);

    scene->setSceneRect(0, 0, ANCHO_ESCENA, ALTO_ESCENA);
    ui->graphicsView->setScene(scene);
    ui->graphicsView->installEventFilter(this);
    ui->graphicsView->setFrameShape(QFrame::NoFrame);
    ui->graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->graphicsView->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    ui->graphicsView->setBackgroundBrush(QBrush(QColor(74, 98, 38)));
    // El teclado lo recibe la ventana (la vista no debe desplazar la escena con las flechas).
    ui->graphicsView->setFocusPolicy(Qt::NoFocus);
    setFocusPolicy(Qt::StrongFocus);

    cargarRecursos();
    construirEscenario();

    juego.iniciar();

    construirHud();
    construirResultado();

    PersonajePrincipal *jugador = juego.getNivel().getJugador();
    vidasAnteriores = jugador ? jugador->getVida() : 3;

    connect(timer, &QTimer::timeout, this, &MainWindow::actualizarJuego);
    timer->setTimerType(Qt::PreciseTimer);
    reloj.start();
    timer->start(16);

    resize(1100, 620);
    setFocus();
    sincronizarJugador();
    sincronizarAnciano();
    sincronizarHud();
    sincronizarResultado();
}

MainWindow::~MainWindow()
{
    timer->stop();
    delete ui;
}

// ---------------------------------------------------------------- recursos

QPixmap MainWindow::cargarEscalado(const QString &ruta, int ancho, int alto) const
{
    const QPixmap original(ruta);
    if (original.isNull()) {
        qWarning() << "No se pudo cargar el recurso" << ruta;
        return QPixmap();
    }
    return original.scaled(ancho, alto, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

// Elimina los márgenes transparentes para que el sprite coincida con su caja de colisión.
QPixmap MainWindow::recortarTransparencia(const QPixmap &origen) const
{
    if (origen.isNull())
        return origen;

    const QImage img = origen.toImage().convertToFormat(QImage::Format_ARGB32);
    int minX = img.width(), minY = img.height(), maxX = -1, maxY = -1;

    for (int y = 0; y < img.height(); ++y) {
        const QRgb *fila = reinterpret_cast<const QRgb*>(img.constScanLine(y));
        for (int x = 0; x < img.width(); ++x) {
            if (qAlpha(fila[x]) > 20) {
                minX = std::min(minX, x);
                maxX = std::max(maxX, x);
                minY = std::min(minY, y);
                maxY = std::max(maxY, y);
            }
        }
    }

    if (maxX < minX || maxY < minY)
        return origen;
    return origen.copy(minX, minY, maxX - minX + 1, maxY - minY + 1);
}

void MainWindow::cargarRecursos()
{
    // Todos los estados del protagonista se recortan y normalizan a un tamaño equivalente.
    pxTerminator = cargarSpriteRecortado(":/terminator.png", 150, 130);
    pxTerminatorAcelerando = cargarSpriteRecortado(":/terminator_acelerando_limpio.png", 150, 130);
    pxTerminatorFrenando = cargarSpriteRecortado(":/terminator_frenando.png", 150, 130);
    pxTerminatorGameOver = cargarSpriteRecortado(":/terminator_gameover.png", 150, 130);
    pxTerminatorPerdioVida = cargarSpriteRecortado(":/terminator_perdio1vida.png", 150, 130);
    pxTerminatorRapidin = cargarSpriteRecortado(":/terminator_rapidin.png", 150, 130);
    pxAnciano  = cargarSpriteRecortado(":/anciano.png", 150, 150);
    pxAnciano2 = cargarSpriteRecortado(":/anciano2.png", 150, 150);
    pxAnciano3 = cargarSpriteRecortado(":/anciano3.png", 150, 150);
    pxAncianoPreparando = cargarSpriteRecortado(":/anciano_preparando_lanzamiento.png", 150, 150);
    pxAncianoLanzando = cargarSpriteRecortado(":/anciano_pleno_lanzamiento.png", 150, 150);
    pxAncianoDespues = cargarSpriteRecortado(":/anciano_ya_lanzo.png", 150, 150);
    pxBotella  = cargarEscalado(":/botella.png", 24, 48);
    pxHueco    = cargarEscalado(":/hueco.png", 100, 42);
    pxCaja     = cargarEscalado(":/caja.png", 64, 64);
    pxCono     = cargarEscalado(":/cono.png", 46, 62);
    pxLlanta   = cargarEscalado(":/llanta.png", 60, 60);
    pxPremio   = cargarEscalado(":/premio.png", 46, 46);
    pxCorazon  = cargarEscalado(":/corazon.png", 36, 36);
    pxPalmera  = cargarEscalado(":/palmera.png", 70, 140);
}

QPixmap MainWindow::cargarSpriteRecortado(const QString &ruta, int ancho, int alto) const
{
    const QPixmap recortado = recortarTransparencia(QPixmap(ruta));
    if (recortado.isNull()) return QPixmap();
    const QPixmap escalado = recortado.scaled(ancho, alto, Qt::KeepAspectRatio,
                                              Qt::SmoothTransformation);
    QPixmap lienzo(ancho, alto);
    lienzo.fill(Qt::transparent);
    QPainter painter(&lienzo);
    painter.drawPixmap((ancho-escalado.width())/2, (alto-escalado.height())/2, escalado);
    painter.end();
    return lienzo;
}

QGraphicsItem *MainWindow::crearSprite(const QPixmap &pixmap, double ancho, double alto,
                                       const QColor &colorRespaldo)
{
    if (!pixmap.isNull())
        return scene->addPixmap(pixmap);

    // Si falta el recurso se dibuja un rectángulo para que el juego siga siendo jugable.
    return scene->addRect(0, 0, ancho, alto, QPen(Qt::black), QBrush(colorRespaldo));
}

QGraphicsTextItem *MainWindow::crearTexto(int tamano, const QColor &color, bool sombra)
{
    auto *texto = scene->addText(QString(), QFont("Arial", tamano, QFont::Bold));
    texto->setDefaultTextColor(color);
    if (sombra) {
        auto *efecto = new QGraphicsDropShadowEffect;
        efecto->setOffset(3, 3);
        efecto->setBlurRadius(5);
        efecto->setColor(QColor(0, 0, 0, 220));
        texto->setGraphicsEffect(efecto);
    }
    return texto;
}

void MainWindow::centrarTexto(QGraphicsTextItem *texto, double centroX, double y)
{
    texto->setPos(centroX - texto->boundingRect().width() / 2.0, y);
}

// ---------------------------------------------------------------- escenario

void MainWindow::construirEscenario()
{
    scene->setBackgroundBrush(QBrush(QColor(74, 98, 38)));
    scene->addRect(-600, -20, ANCHO_ESCENA + 1200, ALTO_ESCENA + 40, Qt::NoPen, QBrush(QColor(74,98,38)))->setZValue(-100);

    // Paisaje: dos copias (normal + espejada) para que el bucle no tenga costura.
    QImage paisaje(":/fondo.png");
    if (!paisaje.isNull()) {
        const int ancho = qRound(paisaje.width() * ALTO_FONDO / paisaje.height());
        paisaje = paisaje.scaled(ancho, qRound(ALTO_FONDO), Qt::IgnoreAspectRatio,
                                 Qt::SmoothTransformation);
        QPixmap compuesto(ancho * 2, paisaje.height());
        QPainter pintor(&compuesto);
        pintor.drawImage(0, 0, paisaje);
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
        pintor.drawImage(ancho, 0, paisaje.flipped(Qt::Horizontal));
#else
        pintor.drawImage(ancho, 0, paisaje.mirrored(true, false));
#endif
        pintor.end();

        anchoFondoCompuesto = compuesto.width();
        fondoA = scene->addPixmap(compuesto);
        fondoB = scene->addPixmap(compuesto);
        fondoA->setZValue(Z_FONDO);
        fondoB->setZValue(Z_FONDO);
    } else {
        qWarning() << "No se pudo cargar :/fondo.png";
    }

    // Bermas (franjas de tierra/hierba sobre y bajo la carretera).
    auto *bermaSuperior = scene->addRect(0, ALTO_FONDO, ANCHO_ESCENA, Y_CARRETERA - ALTO_FONDO,
                                         QPen(Qt::NoPen), QBrush(QColor(98, 122, 48)));
    auto *bermaInferior = scene->addRect(0, Y_CARRETERA + ALTO_CARRETERA, ANCHO_ESCENA,
                                         ALTO_ESCENA - Y_CARRETERA - ALTO_CARRETERA,
                                         QPen(Qt::NoPen), QBrush(QColor(74, 98, 38)));
    bermaSuperior->setZValue(Z_BERMA);
    bermaInferior->setZValue(Z_BERMA);

    // Carretera: mosaico horizontal (asfalto + separadores discontinuos).
    const QPixmap tesela(":/carretera.png");
    QPixmap carretera(int(ANCHO_ESCENA + 2 * ANCHO_TESELA), int(ALTO_CARRETERA));
    carretera.fill(QColor(48, 50, 58));
    if (!tesela.isNull()) {
        QPainter pintor(&carretera);
        pintor.drawTiledPixmap(carretera.rect(), tesela);
        pintor.end();
    } else {
        qWarning() << "No se pudo cargar :/carretera.png";
        carretera.fill(QColor(60, 60, 66));
    }
    carreteraItem = scene->addPixmap(carretera);
    carreteraItem->setPos(0, Y_CARRETERA);
    carreteraItem->setZValue(Z_CARRETERA);

    // Palmeras junto a la carretera (decoración con movimiento más rápido: profundidad).
    for (int i = 0; i < NUM_PALMERAS; ++i) {
        QGraphicsItem *palma = crearSprite(pxPalmera, 70, 140, QColor(30, 120, 40));
        palma->setZValue(Z_PALMERA);
        palmeras.append(palma);
    }

    // Líneas de velocidad (solo visibles con el premio "rapidín").
    for (int i = 0; i < NUM_LINEAS; ++i) {
        const double y = Y_CARRETERA + 20 + std::fmod(i * 97.0, ALTO_CARRETERA - 40);
        auto *linea = scene->addLine(0, y, 110, y, QPen(QColor(255, 255, 255, 150), 3));
        linea->setZValue(Z_LINEAS);
        linea->setVisible(false);
        lineasVelocidad.append(linea);
    }

    // Personajes.
    jugadorItem = crearSprite(pxTerminator, 140, 130, QColor(40, 60, 200));
    jugadorItem->setZValue(Z_ENTIDAD + 1);
    ancianoItem = crearSprite(pxAnciano, 150, 150, QColor(230, 230, 230));
    ancianoItem->setZValue(Z_ANCIANO);
}

void MainWindow::construirHud()
{
    panelHud = scene->addRect(0, 0, ANCHO_ESCENA, 70, QPen(Qt::NoPen), QBrush(QColor(0, 0, 0, 150)));
    panelHud->setZValue(Z_HUD);

    const PersonajePrincipal *protagonista = juego.getNivel().getJugador();
    const int vidasMaximas = protagonista ? protagonista->getVidaMaxima() : 3;
    for (int i = 0; i < vidasMaximas; ++i) {
        QGraphicsItem *corazon = crearSprite(pxCorazon, 36, 36, QColor(220, 30, 50));
        corazon->setPos(20 + i * 44, 17);
        corazon->setZValue(Z_HUD + 1);
        corazones.append(corazon);
    }

    textoTiempo = crearTexto(18, Qt::white);
    textoTiempo->setZValue(Z_HUD + 1);

    barraFondo = scene->addRect(0, 0, 420, 10, QPen(Qt::white, 1), QBrush(QColor(60, 60, 60)));
    barraFondo->setPos(ANCHO_ESCENA / 2.0 - 210, 46);
    barraFondo->setZValue(Z_HUD + 1);
    barraRelleno = scene->addRect(0, 0, 0, 10, QPen(Qt::NoPen), QBrush(QColor(80, 200, 90)));
    barraRelleno->setPos(ANCHO_ESCENA / 2.0 - 210, 46);
    barraRelleno->setZValue(Z_HUD + 2);

    textoEstado = crearTexto(14, QColor(255, 230, 120));
    textoEstado->setZValue(Z_HUD + 1);

    textoRapidin = crearTexto(22, QColor(255, 220, 50), true);
    textoRapidin->setZValue(Z_HUD + 1);

    textoCuenta = crearTexto(96, Qt::white, true);
    textoCuenta->setZValue(Z_HUD + 3);

    textoAyuda = crearTexto(18, Qt::white, true);
    textoAyuda->setZValue(Z_HUD + 3);
}

void MainWindow::construirResultado()
{
    overlay = scene->addRect(0, 0, ANCHO_ESCENA, ALTO_ESCENA, QPen(Qt::NoPen),
                             QBrush(QColor(0, 0, 0, 170)));
    overlay->setZValue(Z_RESULTADO);

    panelResultado = scene->addRect(340, 190, 600, 320, QPen(QColor(240, 200, 60), 4),
                                    QBrush(QColor(28, 28, 36, 235)));
    panelResultado->setZValue(Z_RESULTADO + 1);

    textoResultado = crearTexto(34, Qt::white);
    textoMotivo = crearTexto(17, QColor(235, 235, 235));
    textoDetalle = crearTexto(15, QColor(200, 200, 200));
    textoReiniciar = crearTexto(18, QColor(255, 230, 120));
    for (QGraphicsTextItem *t : {textoResultado, textoMotivo, textoDetalle, textoReiniciar})
        t->setZValue(Z_RESULTADO + 2);

    overlay->setVisible(false);
    panelResultado->setVisible(false);
    textoResultado->setVisible(false);
    textoMotivo->setVisible(false);
    textoDetalle->setVisible(false);
    textoReiniciar->setVisible(false);
}

// ---------------------------------------------------------------- ciclo de juego



void MainWindow::crearEfectoColision(bool fatal)
{
    const PersonajePrincipal *jugador = juego.getNivel().getJugador();
    if (!jugador) return;
    const QPointF centro(jugador->getCentroX(), jugador->getY()+jugador->getAlto()/2.0);
    const int cantidad = fatal ? 12 : 7;
    for (int i=0; i<cantidad; ++i) {
        QPolygonF forma;
        const double radio = fatal ? 8.0 : 6.0;
        for (int k=0; k<10; ++k) {
            const double a=-1.57079632679+k*3.14159265359/5.0;
            const double rr=(k%2==0)?radio:radio*0.42;
            forma << QPointF(std::cos(a)*rr,std::sin(a)*rr);
        }
        auto *estrella=scene->addPolygon(forma,QPen(QColor(255,235,150),1),
                                           QBrush(QColor(255,190,45)));
        estrella->setPos(centro);
        estrella->setZValue(95);
        const double ang=6.28318530718*i/cantidad;
        estrellasColision.append({estrella,QPointF(std::cos(ang)*105.0,
                                                    std::sin(ang)*85.0),0.42});
    }
}

void MainWindow::actualizarEfectosColision(double dt)
{
    for (auto it=estrellasColision.begin(); it!=estrellasColision.end();) {
        it->vida-=dt;
        it->item->moveBy(it->velocidad.x()*dt,it->velocidad.y()*dt);
        it->item->setOpacity(std::max(0.0,it->vida/0.42));
        if (it->vida<=0.0) {
            scene->removeItem(it->item); delete it->item;
            it=estrellasColision.erase(it);
        } else ++it;
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) {
        QMainWindow::keyPressEvent(event);
        return;
    }

    Nivel &nivel = juego.getNivel();
    const int tecla = event->key();

    if (nivel.getEstado() == COMPLETADO || nivel.getEstado() == PERDIDO) {
        if (tecla == Qt::Key_R || tecla == Qt::Key_Return
            || tecla == Qt::Key_Enter || tecla == Qt::Key_Space) {
            reiniciarPartida();
            return;
        }
    } else if (tecla == Qt::Key_W || tecla == Qt::Key_Up) {
        juego.manejarEntrada('W');          // Carril superior.
        return;
    } else if (tecla == Qt::Key_S || tecla == Qt::Key_Down) {
        juego.manejarEntrada('S');          // Carril inferior.
        return;
    }

    QMainWindow::keyPressEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    ajustarVista();
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    ajustarVista();
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->graphicsView && event->type() == QEvent::Resize) {
        ajustarVista();
    }

    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::ajustarVista()
{
    if (!ui->graphicsView || !scene)
        return;

    ui->graphicsView->setTransformationAnchor(QGraphicsView::AnchorViewCenter);
    ui->graphicsView->setResizeAnchor(QGraphicsView::AnchorViewCenter);
    ui->graphicsView->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
}

void MainWindow::reiniciarPartida()
{
    juego.iniciar(); // Nivel::cargar() deja jugador, ángel, objetos y tiempos en limpio.
    estadoAnterior = PREPARACION; tiempoImpacto = 0.0;

    for (auto it = itemsEntidades.begin(); it != itemsEntidades.end(); ++it) {
        scene->removeItem(it.value());
        delete it.value();
    }
    itemsEntidades.clear();
    for (auto &estrella : estrellasColision) {
        scene->removeItem(estrella.item);
        delete estrella.item;
    }
    estrellasColision.clear();

    scrollRuta = 0.0;
    tiempoParpadeo = 0.0;
    PersonajePrincipal *jugador = juego.getNivel().getJugador();
    vidasAnteriores = jugador ? jugador->getVida() : 3;
    reloj.restart();
}

void MainWindow::actualizarJuego()
{
    // Se limita dt para que un tirón (p. ej. al mover la ventana) no atraviese objetos.
    const double dt = std::min(reloj.restart() / 1000.0, 0.05);

    juego.actualizar(dt);
    tiempoAnimacion += dt;

    // El nivel informa qué pasó (colisiones, premios, fin...) y el audio lo convierte en sonido.
    for (const EventoJuego evento : juego.getNivel().tomarEventos())
        audio->procesarEvento(evento);

    actualizarEfectosColision(dt);
    actualizarEscenario(dt);
    sincronizarJugador();
    sincronizarAnciano();
    sincronizarEntidades();
    sincronizarHud();
    sincronizarResultado();
}

// ---------------------------------------------------------------- sincronización

void MainWindow::actualizarEscenario(double dt)
{
    const Nivel &nivel = juego.getNivel();
    const bool enMarcha = nivel.getEstado() == PREPARACION || nivel.getEstado() == JUGANDO;
    if (enMarcha)
        scrollRuta += nivel.getVelocidadScroll() * dt;

    // Carretera: desplazamiento cíclico del mosaico.
    if (carreteraItem)
        carreteraItem->setX(-std::fmod(scrollRuta, ANCHO_TESELA));

    // Paisaje con paralaje.
    if (fondoA && fondoB) {
        const double desplazamiento = std::fmod(scrollRuta * PARALAJE_FONDO, anchoFondoCompuesto);
        fondoA->setPos(-desplazamiento, 0);
        fondoB->setPos(-desplazamiento + anchoFondoCompuesto, 0);
    }

    // Palmeras.
    const double vuelta = NUM_PALMERAS * SEPARACION_PALMERAS;
    for (int i = 0; i < palmeras.size(); ++i) {
        double x = std::fmod(i * SEPARACION_PALMERAS - scrollRuta, vuelta);
        if (x < 0)
            x += vuelta;
        palmeras[i]->setPos(x - 100.0, Y_CARRETERA - 8.0 - palmeras[i]->boundingRect().height()
                                           + 10.0 * (i % 2));
    }

    // Líneas de velocidad (solo con el premio activo).
    PersonajePrincipal *jugador = nivel.getJugador();
    const bool rapidin = enMarcha && jugador && jugador->getEstado() == RAPIDIN;
    for (int i = 0; i < lineasVelocidad.size(); ++i) {
        lineasVelocidad[i]->setVisible(rapidin);
        if (rapidin) {
            double x = std::fmod(i * 173.0 - scrollRuta * 1.8, 1400.0);
            if (x < 0)
                x += 1400.0;
            lineasVelocidad[i]->setX(x - 120.0);
        }
    }
}

void MainWindow::sincronizarJugador()
{
    PersonajePrincipal *jugador = juego.getNivel().getJugador();
    if (!jugador || !jugadorItem)
        return;

    const QRectF r = jugadorItem->boundingRect();
    const bool enMarcha = juego.getNivel().getEstado() == PREPARACION
                          || juego.getNivel().getEstado() == JUGANDO;

    // Vibración de la moto en marcha: transmite que Terminator va conduciendo.
    const double vibracion = enMarcha ? std::sin(tiempoAnimacion * 38.0) * 1.6 : 0.0;

    // Las ruedas quedan apoyadas en la parte baja de la caja de colisión.
    const double x = jugador->getCentroX() - r.width() / 2.0;
    const double y = jugador->getY() + jugador->getAlto() + 14.0 - r.height() + vibracion;
    jugadorItem->setPos(x, y);

    // Inclinación al cambiar de carril (el morro baja o sube según el movimiento).
    const double restante = jugador->getYCarril(jugador->getCarril()) - jugador->getY();
    jugadorItem->setTransformOriginPoint(r.center());
    const double inclinacionChoque = jugador->estaEnRetroceso() ? -18.0 : 0.0;
    jugadorItem->setRotation(std::clamp(restante * 0.12, -8.0, 8.0) + inclinacionChoque);

    jugadorItem->setZValue(Z_ENTIDAD + jugador->getCentroY() / 1000.0 + 0.5);

    // Parpadeo al recibir daño.
    if (jugador->getVida() < vidasAnteriores) {
        tiempoParpadeo = 0.9;
        tiempoImpacto = 0.75;
        crearEfectoColision(false);
    }
    vidasAnteriores = jugador->getVida();

    const EstadoNivel estadoNivel = juego.getNivel().getEstado();
    estadoAnterior = estadoNivel;

    if (jugador->estaEnRetroceso() && !efectoChoqueActivo) {
        crearEfectoColision(true);
        efectoChoqueActivo = true;
    }
    if (!jugador->estaEnRetroceso()) efectoChoqueActivo = false;

    if (tiempoImpacto > 0.0) tiempoImpacto -= 0.016;
    QPixmap sprite = pxTerminator;
    if (estadoNivel == PERDIDO || !jugador->estaActiva()) sprite = pxTerminatorGameOver;
    else if (jugador->estaEnRetroceso()) sprite = pxTerminatorPerdioVida;
    else if (tiempoImpacto > 0.0) sprite = pxTerminatorPerdioVida;
    else if (jugador->getEstado() == RAPIDIN) {
        if (jugador->getTiempoEstado() > 3.8) sprite = pxTerminatorAcelerando;
        else if (jugador->getTiempoEstado() < 0.85) sprite = pxTerminatorFrenando;
        else sprite = pxTerminatorRapidin;
    }
    if (auto *pix = dynamic_cast<QGraphicsPixmapItem*>(jugadorItem))
        if (!sprite.isNull()) pix->setPixmap(sprite);

    if (tiempoParpadeo > 0.0) {
        tiempoParpadeo -= 0.016;
        jugadorItem->setOpacity(std::fmod(tiempoParpadeo * 12.0, 2.0) < 1.0 ? 0.3 : 1.0);
    } else {
        jugadorItem->setOpacity(1.0);
    }
}

void MainWindow::sincronizarAnciano()
{
    const Anciano *anciano = juego.getNivel().getAnciano();
    if (!anciano || !ancianoItem) return;
    const QRectF r=ancianoItem->boundingRect();
    QPixmap frame=pxAnciano;
    if(anciano->getLanzamientoActivo()) {
        const double t=anciano->getTiempoDesdeLanzamiento();
        if(t<0.20) frame=pxAncianoPreparando;
        else if(t<0.42) frame=pxAncianoLanzando;
        else frame=pxAncianoDespues;
    } else {
        const int fase=static_cast<int>(tiempoAnimacion/0.18)%3;
        frame=(fase==0?pxAnciano:(fase==1?pxAnciano2:pxAnciano3));
    }
    if(auto *pix=dynamic_cast<QGraphicsPixmapItem*>(ancianoItem))
        if(!frame.isNull()) pix->setPixmap(frame);
    const double aleteo=std::sin(tiempoAnimacion*7.0);
    ancianoItem->setPos(anciano->getCentroX()-r.width()/2.0,
                        anciano->getCentroY()-r.height()/2.0+aleteo*3.0);
    ancianoItem->setTransformOriginPoint(r.center());
    ancianoItem->setRotation(aleteo*1.5);
}

void MainWindow::sincronizarEntidades()
{
    const auto &entidades = juego.getNivel().getEntidades();
    QSet<const Entidades*> vigentes;

    for (const Entidades *entidad : entidades) {
        if (!entidad || !entidad->estaActiva())
            continue;
        vigentes.insert(entidad);

        QGraphicsItem *item = itemsEntidades.value(entidad, nullptr);
        if (!item) {
            if (const auto *obst = dynamic_cast<const ObjetosEstaticos*>(entidad)) {
                switch (obst->getTipo()) {
                case CAJA:   item = crearSprite(pxCaja, 64, 64, QColor(180, 120, 60)); break;
                case CONO:   item = crearSprite(pxCono, 46, 62, QColor(240, 120, 30)); break;
                case LLANTA: item = crearSprite(pxLlanta, 60, 60, QColor(30, 30, 30)); break;
                }
            } else if (const auto *proyectil = dynamic_cast<const Proyectil*>(entidad)) {
                if (proyectil->esBotella())
                    item = crearSprite(pxBotella, 24, 48, QColor(40, 150, 70));
            } else if (dynamic_cast<const ObjetosArmas*>(entidad)) {
                item = crearSprite(pxHueco, 100, 42, QColor(20, 20, 20));
            } else if (dynamic_cast<const Premio*>(entidad)) {
                item = crearSprite(pxPremio, 46, 46, QColor(255, 210, 40));
            }

            if (!item)
                continue;
            item->setTransformOriginPoint(item->boundingRect().center());
            itemsEntidades.insert(entidad, item);
        }

        const QRectF r = item->boundingRect();
        item->setPos(entidad->getCentroX() - r.width() / 2.0,
                     entidad->getCentroY() - r.height() / 2.0);

        if (dynamic_cast<const Proyectil*>(entidad)) {
            item->setZValue(Z_BOTELLA);
            item->setRotation(tiempoAnimacion * 520.0);         // La botella gira en el aire.
        } else if (dynamic_cast<const Premio*>(entidad)) {
            item->setZValue(Z_ENTIDAD + entidad->getCentroY() / 1000.0);
            item->setScale(1.0 + 0.10 * std::sin(tiempoAnimacion * 8.0));
        } else {
            // Los objetos de carriles inferiores se dibujan encima de los superiores.
            item->setZValue(Z_ENTIDAD + entidad->getCentroY() / 1000.0);
        }
    }

    // Elimina los gráficos de entidades que ya no existen.
    for (auto it = itemsEntidades.begin(); it != itemsEntidades.end(); ) {
        if (!vigentes.contains(it.key())) {
            scene->removeItem(it.value());
            delete it.value();
            it = itemsEntidades.erase(it);
        } else {
            ++it;
        }
    }
}

void MainWindow::sincronizarHud()
{
    const Nivel &nivel = juego.getNivel();
    const PersonajePrincipal *jugador = nivel.getJugador();
    const EstadoNivel estado = nivel.getEstado();

    // Vidas.
    const int vidas = jugador ? jugador->getVida() : 0;
    for (int i = 0; i < corazones.size(); ++i)
        corazones[i]->setOpacity(i < vidas ? 1.0 : 0.2);

    // Tiempo restante y barra de progreso.
    const double duracion = nivel.getDuracionNivel();
    const double restante = std::max(0.0, duracion - nivel.getTiempo());
    textoTiempo->setPlainText(QString("Tiempo restante: %1 s").arg(int(std::ceil(restante))));
    centrarTexto(textoTiempo, ANCHO_ESCENA / 2.0, 8);
    barraRelleno->setRect(0, 0, 420.0 * std::min(1.0, nivel.getTiempo() / duracion), 10);

    // Estado del nivel.
    QString etiqueta;
    switch (estado) {
    case PREPARACION: etiqueta = "Preparación"; break;
    case JUGANDO:     etiqueta = "En ruta a Macondo"; break;
    case COMPLETADO:  etiqueta = "Nivel completado"; break;
    case PERDIDO:     etiqueta = "Nivel perdido"; break;
    }
    textoEstado->setPlainText(etiqueta);
    textoEstado->setPos(ANCHO_ESCENA - textoEstado->boundingRect().width() - 20, 20);

    // Premio rapidín.
    const bool rapidin = estado == JUGANDO && jugador && jugador->getEstado() == RAPIDIN;
    textoRapidin->setVisible(rapidin);
    if (rapidin) {
        textoRapidin->setPlainText(QString("¡RAPIDÍN!  %1 s")
                                       .arg(jugador->getTiempoEstado(), 0, 'f', 1));
        centrarTexto(textoRapidin, ANCHO_ESCENA / 2.0, 78);
    }

    // Cuenta regresiva y mensaje "¡YA!".
    if (estado == PREPARACION) {
        textoCuenta->setVisible(true);
        textoCuenta->setPlainText(QString::number(
            std::max(1, int(std::ceil(nivel.getTiempoPreparacion())))));
        centrarTexto(textoCuenta, ANCHO_ESCENA / 2.0, 120);

        textoAyuda->setVisible(true);
        textoAyuda->setPlainText("¡Prepárate!  W = carril superior   S = carril inferior");
        centrarTexto(textoAyuda, ANCHO_ESCENA / 2.0, 270);
    } else if (estado == JUGANDO && nivel.getTiempo() < 0.9) {
        textoCuenta->setVisible(true);
        textoCuenta->setPlainText("¡YA!");
        centrarTexto(textoCuenta, ANCHO_ESCENA / 2.0, 120);
        textoAyuda->setVisible(false);
    } else {
        textoCuenta->setVisible(false);
        textoAyuda->setVisible(false);
    }
}

void MainWindow::sincronizarResultado()
{
    const Nivel &nivel = juego.getNivel();
    const bool terminado = nivel.getEstado() == COMPLETADO || nivel.getEstado() == PERDIDO;

    overlay->setVisible(terminado);
    panelResultado->setVisible(terminado);
    textoResultado->setVisible(terminado);
    textoMotivo->setVisible(terminado);
    textoDetalle->setVisible(terminado);
    textoReiniciar->setVisible(terminado);

    if (!terminado)
        return;

    const int duracion = int(nivel.getDuracionNivel());
    if (nivel.completado()) {
        textoResultado->setDefaultTextColor(QColor(120, 230, 120));
        textoResultado->setPlainText("¡NIVEL COMPLETADO!");
        textoMotivo->setPlainText(QString("¡Felicidades! Terminator sobrevivió los %1 segundos")
                                      .arg(duracion));
        textoDetalle->setPlainText("de recorrido y llegó a Macondo.");
    } else {
        textoResultado->setDefaultTextColor(QColor(240, 90, 80));
        textoResultado->setPlainText("¡NIVEL PERDIDO!");
        if (nivel.getMotivoDerrota() == OBSTACULO_ESTATICO)
            textoMotivo->setPlainText("Terminator chocó con un obstáculo fijo.");
        else
            textoMotivo->setPlainText("Terminator se quedó sin vidas.");
        textoDetalle->setPlainText(QString("Sobrevivió %1 de %2 segundos.")
                                       .arg(int(nivel.getTiempo())).arg(duracion));
    }
    textoReiniciar->setPlainText("Presiona R (o Enter) para reiniciar el Nivel 1");

    const double cx = ANCHO_ESCENA / 2.0;
    centrarTexto(textoResultado, cx, 225);
    centrarTexto(textoMotivo, cx, 310);
    centrarTexto(textoDetalle, cx, 345);
    centrarTexto(textoReiniciar, cx, 440);
}