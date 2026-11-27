#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "PersonajePrincipal.h"
#include "Anciano.h"
#include "ObjetosEstaticos.h"
#include "ObjetosArmas.h"
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

// Interruptor del sonido siempre está activo; en el futuro puede venir de una opción.
constexpr bool AUDIO_ACTIVO = true;

constexpr double Z_FONDO = 0, Z_BERMA = 1, Z_CARRETERA = 2, Z_PALMERA = 3,
    Z_ENTIDAD = 10, Z_ANCIANO = 30, Z_BOTELLA = 31,
    Z_LINEAS = 40, Z_HUD = 60, Z_RESULTADO = 80;

// Tiempo máximo de vida de una estrella del choque (segundos).
constexpr double VIDA_ESTRELLA = 0.42;

// Solo cambia la visibilidad si hace falta (evita invalidar el ítem en cada frame).
void mostrar(QGraphicsItem *item, bool visible)
{
    if (item && item->isVisible() != visible)
        item->setVisible(visible);
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow),
    scene(new QGraphicsScene(this)), timer(new QTimer(this)),
    fondoVerdeBase(nullptr), cieloSuperior(nullptr),
    bermaSuperior(nullptr), bermaInferior(nullptr),
    fondoA(nullptr), fondoB(nullptr), carreteraItem(nullptr),
    anchoFondoCompuesto(0.0), carreteraBaseX(0.0),
    areaVisibleScene(0.0, 0.0, ANCHO_ESCENA, ALTO_ESCENA),
    jugadorItem(nullptr), ancianoItem(nullptr),
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

    // Casi todo se mueve en cada frame: un índice espacial solo estorba, y repintar la
    // vista completa es más barato que calcular regiones sucias.
    scene->setItemIndexMethod(QGraphicsScene::NoIndex);
    scene->setSceneRect(0, 0, ANCHO_ESCENA, ALTO_ESCENA);
    ui->graphicsView->setScene(scene);
    ui->graphicsView->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
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

    // Si falta un recurso de las entidades se usa un rectángulo de color del mismo tamaño,
    // para que el juego siga siendo jugable (los sprites 1 a 1 siempre son QPixmap).
    auto conRespaldo = [](QPixmap &px, int ancho, int alto, const QColor &color) {
        if (!px.isNull())
            return;
        px = QPixmap(ancho, alto);
        px.fill(color);
    };
    conRespaldo(pxBotella, 24, 48, QColor(40, 150, 70));
    conRespaldo(pxHueco, 100, 42, QColor(20, 20, 20));
    conRespaldo(pxCaja, 64, 64, QColor(180, 120, 60));
    conRespaldo(pxCono, 46, 62, QColor(240, 120, 30));
    conRespaldo(pxLlanta, 60, 60, QColor(30, 30, 30));
    conRespaldo(pxPremio, 46, 46, QColor(255, 210, 40));
}

const QPixmap &MainWindow::pixmapDeEstatico(TipoObjetoEstatico tipo) const
{
    switch (tipo) {
    case CAJA:   return pxCaja;
    case CONO:   return pxCono;
    case LLANTA: return pxLlanta;
    case PREMIO: return pxPremio;
    }
    return pxCaja;
}

const QPixmap &MainWindow::pixmapDeArma(TipoArma tipo) const
{
    return tipo == BOTELLA ? pxBotella : pxHueco;
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
    fondoVerdeBase = scene->addRect(-600, -20, ANCHO_ESCENA + 1200, ALTO_ESCENA + 40,
                                    Qt::NoPen, QBrush(QColor(74, 98, 38)));
    fondoVerdeBase->setZValue(-100);

    // Paisaje: dos copias (normal + espejada) para que el bucle no tenga costura.
    QImage paisaje(":/fondo.png");
    QColor colorCielo(70, 56, 86);
    if (!paisaje.isNull()) {
        colorCielo = paisaje.pixelColor(paisaje.width() / 2, 0);
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

    cieloSuperior = scene->addRect(0, -100, ANCHO_ESCENA, 102, Qt::NoPen, QBrush(colorCielo));
    cieloSuperior->setZValue(-90);
    cieloSuperior->setVisible(false);

    // Bermas (franjas de tierra/hierba sobre y bajo la carretera).
    bermaSuperior = scene->addRect(0, ALTO_FONDO, ANCHO_ESCENA, Y_CARRETERA - ALTO_FONDO,
                                   QPen(Qt::NoPen), QBrush(QColor(98, 122, 48)));
    bermaInferior = scene->addRect(0, Y_CARRETERA + ALTO_CARRETERA, ANCHO_ESCENA,
                                   ALTO_ESCENA - Y_CARRETERA - ALTO_CARRETERA,
                                   QPen(Qt::NoPen), QBrush(QColor(74, 98, 38)));
    bermaSuperior->setZValue(Z_BERMA);
    bermaInferior->setZValue(Z_BERMA);

    // Carretera: mosaico horizontal (asfalto + separadores discontinuos).
    teselaCarretera = QPixmap(":/carretera.png");
    carreteraBaseX = -ANCHO_TESELA;
    QPixmap carretera(int(ANCHO_ESCENA + 3 * ANCHO_TESELA), int(ALTO_CARRETERA));
    carretera.fill(QColor(48, 50, 58));
    if (!teselaCarretera.isNull()) {
        QPainter pintor(&carretera);
        pintor.drawTiledPixmap(carretera.rect(), teselaCarretera);
        pintor.end();
    } else {
        qWarning() << "No se pudo cargar :/carretera.png";
        carretera.fill(QColor(60, 60, 66));
    }
    carreteraItem = scene->addPixmap(carretera);
    carreteraItem->setPos(carreteraBaseX, Y_CARRETERA);
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

    // Un sprite por casilla del Nivel, creado una sola vez y oculto hasta que la casilla se active.
    for (int i = 0; i < Nivel::MAX_ESTATICOS; ++i) {
        graficosEstaticos[i] = scene->addPixmap(QPixmap());
        graficosEstaticos[i]->setVisible(false);
    }
    for (int i = 0; i < Nivel::MAX_ARMAS; ++i) {
        graficosArmas[i] = scene->addPixmap(QPixmap());
        graficosArmas[i]->setVisible(false);
    }

    // Estrellas del choque: se crean una vez y se reciclan.
    for (int i = 0; i < MAX_ESTRELLAS; ++i) {
        auto *estrella = scene->addPolygon(QPolygonF(), QPen(QColor(255, 235, 150), 1),
                                           QBrush(QColor(255, 190, 45)));
        estrella->setZValue(95);
        estrella->setVisible(false);
        estrellas[i].item = estrella;
        estrellas[i].vida = 0.0;
    }
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
    // El mensaje de ayuda nunca cambia: se escribe y se centra una sola vez.
    textoAyuda->setPlainText("¡Prepárate!  W = carril superior   S = carril inferior");
    centrarTexto(textoAyuda, ANCHO_ESCENA / 2.0, 270);
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

    // La forma de la estrella es la misma para todas: se calcula una sola vez.
    QPolygonF forma;
    const double radio = fatal ? 8.0 : 6.0;
    for (int k=0; k<10; ++k) {
        const double a=-1.57079632679+k*3.14159265359/5.0;
        const double rr=(k%2==0)?radio:radio*0.42;
        forma << QPointF(std::cos(a)*rr,std::sin(a)*rr);
    }

    // Se reciclan las estrellas libres (vida <= 0); si no quedan, el efecto sale con menos.
    int usadas = 0;
    for (int i=0; i<MAX_ESTRELLAS && usadas<cantidad; ++i) {
        EstrellaVisual &estrella = estrellas[i];
        if (estrella.vida > 0.0) continue;
        const double ang=6.28318530718*usadas/cantidad;
        estrella.item->setPolygon(forma);
        estrella.item->setPos(centro);
        estrella.item->setOpacity(1.0);
        estrella.item->setVisible(true);
        estrella.velocidad=QPointF(std::cos(ang)*105.0,std::sin(ang)*85.0);
        estrella.vida=VIDA_ESTRELLA;
        ++usadas;
    }
}

void MainWindow::actualizarEfectosColision(double dt)
{
    for (int i=0; i<MAX_ESTRELLAS; ++i) {
        EstrellaVisual &estrella = estrellas[i];
        if (estrella.vida <= 0.0) continue;
        estrella.vida-=dt;
        if (estrella.vida<=0.0) {
            estrella.item->setVisible(false);      // casilla libre para el próximo choque
            continue;
        }
        estrella.item->moveBy(estrella.velocidad.x()*dt,estrella.velocidad.y()*dt);
        estrella.item->setOpacity(estrella.vida/VIDA_ESTRELLA);
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
    adaptarEscenarioAVista();
}

void MainWindow::adaptarEscenarioAVista()
{
    if (!ui->graphicsView || !ui->graphicsView->viewport() || !scene)
        return;

    QRectF vis = ui->graphicsView->mapToScene(ui->graphicsView->viewport()->rect()).boundingRect();
    if (vis.isEmpty() || vis.width() <= 0.0 || vis.height() <= 0.0)
        vis = scene->sceneRect();
    areaVisibleScene = vis;

    const double izq = std::min(0.0, vis.left()) - 64.0;
    const double der = std::max(ANCHO_ESCENA, vis.right()) + 64.0;
    const double arriba = std::min(0.0, vis.top()) - 64.0;
    const double abajo = std::max(ALTO_ESCENA, vis.bottom()) + 64.0;
    const double anchoTotal = der - izq;

    if (fondoVerdeBase)
        fondoVerdeBase->setRect(izq, arriba, anchoTotal, abajo - arriba);

    if (cieloSuperior) {
        if (vis.top() < 0.0) {
            cieloSuperior->setRect(izq, arriba, anchoTotal, -arriba + 2.0);
            cieloSuperior->setVisible(true);
        } else {
            cieloSuperior->setVisible(false);
        }
    }

    if (bermaSuperior)
        bermaSuperior->setRect(izq, ALTO_FONDO, anchoTotal, Y_CARRETERA - ALTO_FONDO);
    if (bermaInferior)
        bermaInferior->setRect(izq, Y_CARRETERA + ALTO_CARRETERA,
                               anchoTotal, abajo - (Y_CARRETERA + ALTO_CARRETERA));

    // Mantener siempre >= 1 tesela completa a la izquierda del borde visible y >= 1 a la derecha.
    carreteraBaseX = std::floor(vis.left() / ANCHO_TESELA) * ANCHO_TESELA - ANCHO_TESELA;
    const double anchoNecesario = (der - carreteraBaseX) + 2.0 * ANCHO_TESELA;
    const int numTeselas = std::max(8, int(std::ceil(anchoNecesario / ANCHO_TESELA)));
    const int anchoReq = numTeselas * int(ANCHO_TESELA);

    if (carreteraItem && carreteraItem->pixmap().width() < anchoReq) {
        QPixmap carretera(anchoReq, int(ALTO_CARRETERA));
        carretera.fill(QColor(48, 50, 58));
        if (!teselaCarretera.isNull()) {
            QPainter pintor(&carretera);
            pintor.drawTiledPixmap(carretera.rect(), teselaCarretera);
            pintor.end();
        } else {
            carretera.fill(QColor(60, 60, 66));
        }
        carreteraItem->setPixmap(carretera);
    }

    // Asegura suficientes palmeras si la pantalla es más ancha que el escenario base.
    const double anchoPalmas = std::max(NUM_PALMERAS * SEPARACION_PALMERAS,
                                        anchoTotal + 2.0 * SEPARACION_PALMERAS);
    const int palmasRequeridas = std::max(NUM_PALMERAS, int(std::ceil(anchoPalmas / SEPARACION_PALMERAS)));
    while (palmeras.size() < palmasRequeridas) {
        QGraphicsItem *palma = crearSprite(pxPalmera, 70, 140, QColor(30, 120, 40));
        palma->setZValue(Z_PALMERA);
        palmeras.append(palma);
    }

    if (panelHud)
        panelHud->setRect(izq, arriba, anchoTotal, 70.0 - arriba);
    if (overlay)
        overlay->setRect(izq, arriba, anchoTotal, abajo - arriba);

    actualizarEscenario(0.0);
    sincronizarHud();
}

void MainWindow::reiniciarPartida()
{
    juego.iniciar(); // Nivel::cargar() deja jugador, ángel, objetos y tiempos en limpio.
    estadoAnterior = PREPARACION; tiempoImpacto = 0.0;

    // Los sprites no se destruyen: solo se ocultan hasta que sus casillas vuelvan a activarse.
    for (int i = 0; i < Nivel::MAX_ESTATICOS; ++i)
        graficosEstaticos[i]->setVisible(false);
    for (int i = 0; i < Nivel::MAX_ARMAS; ++i)
        graficosArmas[i]->setVisible(false);
    for (int i = 0; i < MAX_ESTRELLAS; ++i) {
        estrellas[i].vida = 0.0;
        estrellas[i].item->setVisible(false);
    }

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
    Nivel &nivel = juego.getNivel();
    for (int i = 0; i < nivel.getNumEventos(); ++i)
        audio->procesarEvento(nivel.getEvento(i));
    nivel.limpiarEventos();

    actualizarEfectosColision(dt);
    actualizarEscenario(dt);
    sincronizarJugador(dt);
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

    // Carretera: desplazamiento cíclico del mosaico adaptado al borde visible.
    if (carreteraItem)
        carreteraItem->setX(carreteraBaseX - std::fmod(scrollRuta, ANCHO_TESELA));

    // Paisaje con paralaje sin costuras incluso en x < 0.
    if (fondoA && fondoB && anchoFondoCompuesto > 0.0) {
        const double base = -std::fmod(scrollRuta * PARALAJE_FONDO, anchoFondoCompuesto);
        const double k = std::floor((areaVisibleScene.left() - base) / anchoFondoCompuesto);
        const double x0 = base + k * anchoFondoCompuesto;
        fondoA->setPos(x0, 0);
        fondoB->setPos(x0 + anchoFondoCompuesto, 0);
    }

    // Palmeras.
    const double xInicioPalmas = std::min(0.0, areaVisibleScene.left()) - 100.0;
    const double vuelta = std::max(1, int(palmeras.size())) * SEPARACION_PALMERAS;
    for (int i = 0; i < palmeras.size(); ++i) {
        double x = std::fmod(i * SEPARACION_PALMERAS - scrollRuta - xInicioPalmas, vuelta);
        if (x < 0)
            x += vuelta;
        palmeras[i]->setPos(xInicioPalmas + x,
                            Y_CARRETERA - 8.0 - palmeras[i]->boundingRect().height()
                                + 10.0 * (i % 2));
    }

    // Líneas de velocidad (solo con el premio activo).
    const PersonajePrincipal *jugador = nivel.getJugador();
    const bool rapidin = enMarcha && jugador && jugador->getEstado() == RAPIDIN;
    const double xInicioLineas = std::min(0.0, areaVisibleScene.left()) - 120.0;
    const double xFinLineas = std::max(ANCHO_ESCENA, areaVisibleScene.right()) + 120.0;
    const double vueltaLineas = std::max(1400.0, xFinLineas - xInicioLineas);
    for (int i = 0; i < lineasVelocidad.size(); ++i) {
        lineasVelocidad[i]->setVisible(rapidin);
        if (rapidin) {
            double x = std::fmod(i * 173.0 - scrollRuta * 1.8 - xInicioLineas, vueltaLineas);
            if (x < 0)
                x += vueltaLineas;
            lineasVelocidad[i]->setX(xInicioLineas + x);
        }
    }
}

void MainWindow::sincronizarJugador(double dt)
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

    // Inclinación al cambiar de carril.
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

    if (tiempoImpacto > 0.0) tiempoImpacto -= dt;
    QPixmap sprite = pxTerminator;
    if (estadoNivel == PERDIDO || !jugador->estaActiva()) sprite = pxTerminatorGameOver;
    else if (jugador->estaEnRetroceso()) sprite = pxTerminatorPerdioVida;
    else if (tiempoImpacto > 0.0) sprite = pxTerminatorPerdioVida;
    else if (jugador->getEstado() == RAPIDIN) {
        if (jugador->getTiempoEstado() > 3.8) sprite = pxTerminatorAcelerando;
        else if (jugador->getTiempoEstado() < 0.85) sprite = pxTerminatorFrenando;
        else sprite = pxTerminatorRapidin;
    }
    // Solo se cambia el sprite si realmente es otro (cacheKey identifica la imagen).
    if (auto *pix = qgraphicsitem_cast<QGraphicsPixmapItem*>(jugadorItem))
        if (!sprite.isNull() && pix->pixmap().cacheKey() != sprite.cacheKey())
            pix->setPixmap(sprite);

    if (tiempoParpadeo > 0.0) {
        tiempoParpadeo -= dt;
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
    if(auto *pix=qgraphicsitem_cast<QGraphicsPixmapItem*>(ancianoItem))
        if(!frame.isNull() && pix->pixmap().cacheKey()!=frame.cacheKey()) pix->setPixmap(frame);
    const double aleteo=std::sin(tiempoAnimacion*7.0);
    ancianoItem->setPos(anciano->getCentroX()-r.width()/2.0,
                        anciano->getCentroY()-r.height()/2.0+aleteo*3.0);
    ancianoItem->setTransformOriginPoint(r.center());
    ancianoItem->setRotation(aleteo*1.5);
}

// Cada casilla del Nivel tiene su sprite (mismo índice). Casilla activa = sprite visible y
// colocado; casilla libre = sprite oculto. No se crea ni se destruye ningún QGraphicsItem.
void MainWindow::sincronizarEntidades()
{
    const Nivel &nivel = juego.getNivel();

    for (int i = 0; i < Nivel::MAX_ESTATICOS; ++i) {
        const ObjetosEstaticos &objeto = nivel.getEstatico(i);
        QGraphicsPixmapItem *grafico = graficosEstaticos[i];

        if (!objeto.estaActiva()) {
            mostrar(grafico, false);
            continue;
        }

        if (!grafico->isVisible()) {
            // Acaba de activarse: se le da su imagen (puede ser otro tipo que la vez anterior).
            grafico->setPixmap(pixmapDeEstatico(objeto.getTipo()));
            grafico->setTransformOriginPoint(grafico->boundingRect().center());
            grafico->setRotation(0.0);
            grafico->setScale(1.0);
            grafico->setVisible(true);
        }

        const QRectF r = grafico->boundingRect();
        grafico->setPos(objeto.getCentroX() - r.width() / 2.0,
                        objeto.getCentroY() - r.height() / 2.0);
        // Los objetos de carriles inferiores se dibujan encima de los superiores.
        grafico->setZValue(Z_ENTIDAD + objeto.getCentroY() / 1000.0);
        if (objeto.esPremio())
            grafico->setScale(1.0 + 0.10 * std::sin(tiempoAnimacion * 8.0));   // el premio "late"
    }

    for (int i = 0; i < Nivel::MAX_ARMAS; ++i) {
        const ObjetosArmas &arma = nivel.getArma(i);
        QGraphicsPixmapItem *grafico = graficosArmas[i];

        if (!arma.estaActiva()) {
            mostrar(grafico, false);
            continue;
        }

        if (!grafico->isVisible()) {
            grafico->setPixmap(pixmapDeArma(arma.getTipo()));
            grafico->setTransformOriginPoint(grafico->boundingRect().center());
            grafico->setRotation(0.0);
            grafico->setScale(1.0);
            grafico->setVisible(true);
        }

        const QRectF r = grafico->boundingRect();
        grafico->setPos(arma.getCentroX() - r.width() / 2.0,
                        arma.getCentroY() - r.height() / 2.0);
        if (arma.esBotella()) {
            grafico->setZValue(Z_BOTELLA);
            grafico->setRotation(tiempoAnimacion * 520.0);      // La botella gira en el aire.
        } else {
            grafico->setZValue(Z_ENTIDAD + arma.getCentroY() / 1000.0);
        }
    }
}

void MainWindow::sincronizarHud()
{
    const Nivel &nivel = juego.getNivel();
    const PersonajePrincipal *jugador = nivel.getJugador();
    const EstadoNivel estado = nivel.getEstado();

    // Vidas: los corazones solo se tocan si cambian las vidas o el borde izquierdo visible.
    const double bordeIzqHud = std::min(0.0, areaVisibleScene.left());
    const double bordeDerHud = std::max(ANCHO_ESCENA, areaVisibleScene.right());
    const int vidas = jugador ? jugador->getVida() : 0;
    if (vidas != vidasMostradas || bordeIzqHud != bordeIzqMostrado) {
        vidasMostradas = vidas;
        bordeIzqMostrado = bordeIzqHud;
        for (int i = 0; i < corazones.size(); ++i) {
            corazones[i]->setPos(bordeIzqHud + 20.0 + i * 44.0, 17.0);
            corazones[i]->setOpacity(i < vidas ? 1.0 : 0.2);
        }
    }

    // Tiempo restante: el texto se rehace solo cuando cambia el segundo mostrado.
    const double duracion = nivel.getDuracionNivel();
    const double restante = std::max(0.0, duracion - nivel.getTiempo());
    const int segundos = int(std::ceil(restante));
    if (segundos != segundosMostrados) {
        segundosMostrados = segundos;
        textoTiempo->setPlainText(QString("Tiempo restante: %1 s").arg(segundos));
        centrarTexto(textoTiempo, ANCHO_ESCENA / 2.0, 8);
    }

    // Barra de progreso: solo si cambia en píxeles enteros.
    const int anchoBarra = int(420.0 * std::min(1.0, nivel.getTiempo() / duracion));
    if (anchoBarra != anchoBarraMostrado) {
        anchoBarraMostrado = anchoBarra;
        barraRelleno->setRect(0, 0, anchoBarra, 10);
    }

    // Estado del nivel (y su posición, que depende del borde derecho visible).
    if (int(estado) != estadoMostrado || bordeDerHud != bordeDerMostrado) {
        estadoMostrado = int(estado);
        bordeDerMostrado = bordeDerHud;
        QString etiqueta;
        switch (estado) {
        case PREPARACION: etiqueta = "Preparación"; break;
        case JUGANDO:     etiqueta = "En ruta a Macondo"; break;
        case COMPLETADO:  etiqueta = "Nivel completado"; break;
        case PERDIDO:     etiqueta = "Nivel perdido"; break;
        }
        textoEstado->setPlainText(etiqueta);
        textoEstado->setPos(bordeDerHud - textoEstado->boundingRect().width() - 20.0, 20.0);
    }

    // Premio rapidín: el texto cambia cada décima de segundo.
    const bool rapidin = estado == JUGANDO && jugador && jugador->getEstado() == RAPIDIN;
    mostrar(textoRapidin, rapidin);
    if (rapidin) {
        const int decimas = qRound(jugador->getTiempoEstado() * 10.0);
        if (decimas != decimasRapidinMostradas) {
            decimasRapidinMostradas = decimas;
            textoRapidin->setPlainText(QString("¡RAPIDÍN!  %1 s").arg(decimas / 10.0, 0, 'f', 1));
            centrarTexto(textoRapidin, ANCHO_ESCENA / 2.0, 78);
        }
    } else {
        decimasRapidinMostradas = -1;
    }

    // Cuenta regresiva y mensaje "¡YA!": 0 = oculto, 1..3 = número, 100 = "¡YA!".
    int cuenta = 0;
    if (estado == PREPARACION)
        cuenta = std::max(1, int(std::ceil(nivel.getTiempoPreparacion())));
    else if (estado == JUGANDO && nivel.getTiempo() < 0.9)
        cuenta = 100;

    if (cuenta != cuentaMostrada) {
        cuentaMostrada = cuenta;
        mostrar(textoCuenta, cuenta != 0);
        if (cuenta != 0) {
            textoCuenta->setPlainText(cuenta == 100 ? QString("¡YA!") : QString::number(cuenta));
            centrarTexto(textoCuenta, ANCHO_ESCENA / 2.0, 120);
        }
    }
    mostrar(textoAyuda, estado == PREPARACION);
}

void MainWindow::sincronizarResultado()
{
    const Nivel &nivel = juego.getNivel();
    const bool terminado = nivel.getEstado() == COMPLETADO || nivel.getEstado() == PERDIDO;

    // Solo se actúa cuando la pantalla de resultado aparece o desaparece.
    if (terminado == resultadoVisible)
        return;
    resultadoVisible = terminado;

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
