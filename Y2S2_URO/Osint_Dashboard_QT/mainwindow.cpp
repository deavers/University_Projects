#include "mainwindow.h"
#include "settingsdialog.h"
#include <QApplication>
#include <QIcon>
#include <QRandomGenerator>
#include <QCursor>
#include <QFile>
#include <QCoreApplication>
#include <cmath>
#include <utility>
#include <QImage>
#include <QPixmap>
#include <QKeyEvent>
#include <QShortcut>
#include <QPainterPath>

// ==============================================================================
// UTILITY FUNCTIONS
// ==============================================================================
static QPushButton* makeIconBtn(const QString &iconPath, const QString &fallback, int w, int h, const QString &obj, const QString &tip = "") {
    auto *btn = new QPushButton();
    btn->setObjectName(obj);
    btn->setFixedSize(w, h);
    if (!tip.isEmpty()) btn->setToolTip(tip);
    QIcon ico(iconPath);
    if (!ico.isNull()) {
        btn->setIcon(ico);
        btn->setIconSize(QSize(w-10, h-10));
    } else {
        btn->setText(fallback);
    }
    return btn;
}

// ==============================================================================
// INTERACTIVE IMAGE VIEWER MODULE
// ==============================================================================
InteractiveImageViewer::InteractiveImageViewer(QWidget *parent) : QWidget(parent) {
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TranslucentBackground);
    setFocusPolicy(Qt::StrongFocus);

    zoom = 1.0;
    panX = panY = 0;

    closeBtn = new QPushButton("✕", this);
    closeBtn->setFixedSize(40, 40);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet(
        "QPushButton { background-color: rgba(20, 22, 25, 200); color: #FFF; border-radius: 20px; font-size: 18px; font-weight: bold; border: 2px solid #3C4043; }"
        "QPushButton:hover { background-color: #FF4444; }"
        );
    connect(closeBtn, &QPushButton::clicked, this, &InteractiveImageViewer::closed);
}

void InteractiveImageViewer::setImage(const QPixmap &pixmap) {
    image = pixmap;
    if (!image.isNull()) {
        double zx = (width() * 0.8) / image.width();
        double zy = (height() * 0.8) / image.height();
        zoom = qMin(zx, zy);
        panX = (width() - image.width() * zoom) / 2.0;
        panY = (height() - image.height() * zoom) / 2.0;
    }
    update();
}

void InteractiveImageViewer::wheelEvent(QWheelEvent *event) {
    if (image.isNull()) return;
    double oldZoom = zoom;
    zoom *= (event->angleDelta().y() > 0) ? 1.1 : (1.0 / 1.1);
    zoom = qBound(0.1, zoom, 10.0);

    double scale = zoom / oldZoom;
    QPointF mp = event->position();
    panX = mp.x() - (mp.x() - panX) * scale;
    panY = mp.y() - (mp.y() - panY) * scale;
    update();
}

void InteractiveImageViewer::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) lastMousePos = event->pos();
}

void InteractiveImageViewer::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) {
        panX += event->pos().x() - lastMousePos.x();
        panY += event->pos().y() - lastMousePos.y();
        lastMousePos = event->pos();
        update();
    }
}

void InteractiveImageViewer::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    closeBtn->move(width() - 60, 20);
}

void InteractiveImageViewer::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.fillRect(rect(), QColor(13, 15, 18, 240));

    if (!image.isNull()) {
        p.save();
        p.translate(panX, panY);
        p.scale(zoom, zoom);
        p.drawPixmap(0, 0, image);
        p.restore();
    }
}

// ==============================================================================
// OSINT NODE MAP MODULE
// ==============================================================================
OSINTNodeMap::OSINTNodeMap(QWidget *parent)
    : QWidget(parent), isActive(false),
    panX(0), panY(0), zoom(1.0),
    rotationAngle(0), pulseCounter(0)
{
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    mapBackground.load(":/map.png");
    initParticles();

    pulseTimer = new QTimer(this);
    connect(pulseTimer, &QTimer::timeout, this, [this]() {
        rotationAngle += 0.05;
        if (rotationAngle >= 360.0) rotationAngle -= 360.0;
        pulseCounter++;
        if (pulseCounter >= 240) pulseCounter = 0;

        for (auto &p : bgParticles) {
            p.y -= p.speed;
            if (p.y < -10) {
                p.y = height() + 10;
                p.x = QRandomGenerator::global()->bounded(0, qMax(1, width()));
            }
            p.alpha += QRandomGenerator::global()->bounded(-5, 6);
            p.alpha = qBound(30, p.alpha, 255);
        }
        update();
    });
    pulseTimer->start(16);
}

void OSINTNodeMap::initParticles() {
    bgParticles.clear();
    for (int i = 0; i < 150; ++i) {
        BackgroundParticle p;
        p.x     = QRandomGenerator::global()->bounded(0, 2560);
        p.y     = QRandomGenerator::global()->bounded(0, 1440);
        p.speed = QRandomGenerator::global()->bounded(5, 20) / 40.0;
        p.size  = QRandomGenerator::global()->bounded(10, 25) / 10.0;
        p.alpha = QRandomGenerator::global()->bounded(50, 255);
        bgParticles.append(p);
    }
}

void OSINTNodeMap::resizeEvent(QResizeEvent *e) {
    QWidget::resizeEvent(e);
    if (!isActive && panX == 0 && panY == 0) {
        panX = width()  / 2.0;
        panY = height() / 2.0;
    }
}

void OSINTNodeMap::setMode(bool active) {
    isActive = active;
    selectedNodeTitle.clear();

    // Center map on Europe on case activation
    if (active) {
        zoom = 1.6;
        if (!mapBackground.isNull()) {
            double tW = mapBackground.width() * zoom;
            double tH = mapBackground.height() * zoom;
            panX = (width() / 2.0) - (0.48 * tW);
            panY = (height() / 2.0) - (0.17 * tH);
        }
    }
    update();
}

void OSINTNodeMap::generateRandomCase() {
    if (!nodes.isEmpty()) return;

    OsintNode n1;
    n1.x_frac = 0.458; n1.y_frac = 0.171; n1.color = QColor(0, 255, 65);
    n1.title = "iPhone 14 Pro (EXIF) - London";
    n1.info = "<b>Location:</b> London, UK<br><b>GPS:</b> 51.51°, -0.13°<br><br>"
              "<div style='background-color:#282A2C; padding:10px; border-radius:5px; border: 1px solid #3C4043;'>"
              "<b style='color:#8AB4F8;'>📷 EXIF METADATA</b><br>"
              "<a href='img://:/Photos/London_uaha.jpeg'>"
              "<img src=':/Photos/London_uaha.jpeg' width='220'></a><br>"
              "<i style='color:#9AA0A6;'>IMG_8932.heic (Intercepted)</i><br><br>"
              "<b style='color:#00FF41;'>📊 ACTIVITY GRAPH (24H)</b><br>"
              "<table width='100%' height='60' cellspacing='2'><tr valign='bottom'>"
              "<td><div style='background:#00FF41; height:15px;'></div></td>"
              "<td><div style='background:#00FF41; height:60px;'></div></td>"
              "<td><div style='background:#00FF41; height:50px;'></div></td>"
              "<td><div style='background:#00FF41; height:10px;'></div></td>"
              "<td><div style='background:#00FF41; height:5px;'></div></td>"
              "</tr><tr style='font-size:9px; color:#9AA0A6; text-align:center;'>"
              "<td>18:00</td><td>02:00</td><td>04:00</td><td>10:00</td><td>14:00</td></tr></table></div>";
    nodes.append(n1);

    OsintNode n2;
    n2.x_frac = 0.2732; n2.y_frac = 0.2395; n2.color = QColor(0, 255, 65);
    n2.title = "Web Session - New York";
    n2.info = "<b>Location:</b> New York, USA<br><b>GPS:</b> 40.71°, -74.01°<br><br>"
              "<div style='background-color:#282A2C; padding:10px; border-radius:5px; border: 1px solid #3C4043;'>"
              "<b style='color:#8AB4F8;'>📡 ROUTING MAP</b><br>"
              "<a href='img://:/Photos/NewYork_map.png'>"
              "<img src=':/Photos/NewYork_map.png' width='220'></a><br><br>"
              "<b style='color:#00FF41;'>📊 NETWORK FREQUENCY</b><br>"
              "<ul style='margin-top:5px; margin-bottom:0px; color:#E3E3E3; padding-left:15px;'>"
              "<li><b>Apr 10:</b> 2 hours (Active)</li>"
              "<li><b>Apr 11:</b> 30 mins (Idle)</li>"
              "<li><b>Apr 12:</b> 2 hours (Active)</li></ul></div>";
    nodes.append(n2);

    OsintNode n3;
    n3.x_frac = 0.55; n3.y_frac = 0.146; n3.color = QColor(0, 255, 65);
    n3.title = "Sat-Com Intercept - Moscow";
    n3.info = "<b>Location:</b> Moscow, RU<br><b>GPS:</b> 55.76°, 37.62°<br><br>"
              "<div style='background-color:#282A2C; padding:10px; border-radius:5px; border: 1px solid #3C4043;'>"
              "<b style='color:#8AB4F8;'>🏢 SOURCE: MOSCOW CITY</b><br>"
              "<a href='img://:/Photos/Moscow.png'>"
              "<img src=':/Photos/Moscow.png' width='220'></a><br><br>"
              "<b style='color:#00FF41;'>📊 FREQUENCY IN COMPANY</b><br>"
              "<table width='100%' height='50' cellspacing='2'><tr valign='bottom'>"
              "<td><div style='background:#00FF41; height:15px;'></div></td>"
              "<td><div style='background:#3C4043; height:2px;'></div></td>"
              "<td><div style='background:#00FF41; height:45px;'></div></td>"
              "</tr><tr style='font-size:9px; color:#9AA0A6; text-align:center;'>"
              "<td>10.04</td><td>11.04</td><td>12.04</td></tr></table></div>";
    nodes.append(n3);

    OsintNode n4;
    n4.x_frac = 0.845; n4.y_frac = 0.802; n4.color = QColor(0, 255, 65);
    n4.title = "Android Device - Sydney";
    n4.info = "<b>Location:</b> Sydney, AU<br><b>GPS:</b> -33.87°, 151.21°<br><br>"
              "<div style='background-color:#282A2C; padding:10px; border-radius:5px; border: 1px solid #3C4043;'>"
              "<b style='color:#8AB4F8;'>📡 CELL TOWER TRIANGULATION</b><br>"
              "<a href='img://:/Photos/Sydney_map.png'>"
              "<img src=':/Photos/Sydney_map.png' width='220'></a><br><br>"
              "<b style='color:#00FF41;'>📊 ACTIVITY GRAPH (24H)</b><br>"
              "<table width='100%' height='60' cellspacing='2'><tr valign='bottom'>"
              "<td><div style='background:#00FF41; height:5px;'></div></td>"
              "<td><div style='background:#00FF41; height:45px;'></div></td>"
              "<td><div style='background:#00FF41; height:10px;'></div></td>"
              "<td><div style='background:#00FF41; height:55px;'></div></td>"
              "</tr><tr style='font-size:9px; color:#9AA0A6; text-align:center;'>"
              "<td>08:00</td><td>13:00</td><td>16:00</td><td>20:00</td></tr></table></div>";
    nodes.append(n4);

    OsintNode n5;
    n5.x_frac = 0.325; n5.y_frac = 0.665; n5.color = QColor(0, 255, 65);
    n5.title = "VPN Exit Node - Brasilia";
    n5.info = "<b>Location:</b> Brasilia, BR<br><b>GPS:</b> -15.80°, -47.89°<br><br>"
              "<div style='background-color:#282A2C; padding:15px; border-radius:5px; border: 1px solid #3C4043; text-align:center;'>"
              "<b style='color:#FF4444; font-size:14px;'>⚠ POSSIBLE COMPANY:</b><br>"
              "<b style='color:#E3E3E3; font-size:16px;'>NordVPN</b></div>";
    nodes.append(n5);

    OsintNode n6;
    n6.x_frac = 0.2635; n6.y_frac = 0.22; n6.color = QColor(0, 255, 65);
    n6.title = "Local Network - Toronto";
    n6.info = "<b>Location:</b> Toronto, CA<br><b>GPS:</b> 43.65°, -79.35°<br><b>Status:</b> <span style='color:#00FF41;'>Intercepted</span><br><b>Confidence:</b> 75%<br><br>"
              "<div style='background-color:#282A2C; padding:10px; border-radius:5px; border: 1px solid #3C4043;'>"
              "<b style='color:#8AB4F8;'>📷 STREET VIEW OVERRIDE</b><br>"
              "<a href='img://:/Photos/Toronto_godlike.jpeg'>"
              "<img src=':/Photos/Toronto_godlike.jpeg' width='220'></a></div>";
    nodes.append(n6);

    update();
}

void OSINTNodeMap::wheelEvent(QWheelEvent *e) {
    if (!isActive) return;
    double oldZoom = zoom;
    zoom *= (e->angleDelta().y() > 0) ? 1.1 : (1.0 / 1.1);
    zoom = qBound(0.2, zoom, 10.0);
    double scale = zoom / oldZoom;
    QPointF mp = e->position();
    panX = mp.x() - (mp.x() - panX) * scale;
    panY = mp.y() - (mp.y() - panY) * scale;
    update();
}

void OSINTNodeMap::mousePressEvent(QMouseEvent *e) {
    if (!isActive || mapBackground.isNull()) return;
    lastMousePos = e->pos();

    double tW = mapBackground.width() * zoom;
    double tH = mapBackground.height() * zoom;

    double ox = std::fmod(panX, tW);
    if (ox > 0) ox -= tW;
    double oy = std::fmod(panY, tH);
    if (oy > 0) oy -= tH;

    for (double x = ox; x < width(); x += tW) {
        for (double y = oy; y < height(); y += tH) {
            for (const auto &node : nodes) {
                QPointF sp(x + node.x_frac * tW, y + node.y_frac * tH);
                if (QLineF(e->pos(), sp).length() < 15.0 * zoom) {
                    selectedNodeTitle = node.title;
                    emit nodeClicked(node.title, node.info);
                    update();
                    return;
                }
            }
        }
    }
}

void OSINTNodeMap::mouseMoveEvent(QMouseEvent *e) {
    if (!isActive || !(e->buttons() & Qt::LeftButton)) return;
    panX += e->pos().x() - lastMousePos.x();
    panY += e->pos().y() - lastMousePos.y();
    lastMousePos = e->pos();
    update();
}

void OSINTNodeMap::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // ── HOME SCREEN ────────────────────────────────
    if (!isActive) {
        p.fillRect(rect(), QColor(20, 22, 25, 255));

        p.setPen(Qt::NoPen);
        for (const auto &part : std::as_const(bgParticles)) {
            p.setBrush(QColor(255, 255, 255, part.alpha));
            p.drawEllipse(QPointF(part.x, part.y), part.size, part.size);
        }

        int R = qMin(width(), height()) / 3;
        QPointF center(width() / 2.0, height() / 2.0);

        QRadialGradient glow(center, R * 1.3);
        glow.setColorAt(0, QColor(74, 144, 226, 50));
        glow.setColorAt(1, QColor(74, 144, 226, 0));
        p.setBrush(glow);
        p.setPen(Qt::NoPen);
        p.drawEllipse(center, R * 1.3, R * 1.3);

        QPainterPath globePath;
        globePath.addEllipse(center, R, R);
        p.setClipPath(globePath);

        p.fillPath(globePath, QColor(15, 20, 30, 255));

        p.setPen(QPen(QColor(74, 144, 226, 90), 1.5));
        for (int lat = -75; lat <= 75; lat += 15) {
            double y_offset = R * std::sin(lat * M_PI / 180.0);
            double x_width = R * std::cos(lat * M_PI / 180.0);
            p.drawLine(QPointF(center.x() - x_width, center.y() + y_offset),
                       QPointF(center.x() + x_width, center.y() + y_offset));
        }

        for (int i = 0; i < 18; ++i) {
            double ang = (rotationAngle + i * 20.0) * M_PI / 180.0;
            double rx  = R * std::cos(ang);
            p.drawEllipse(QRectF(center.x() - std::abs(rx), center.y() - R, 2 * std::abs(rx), 2 * R));
        }

        p.setClipping(false);

        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(QColor(74, 144, 226, 150), 2.0));
        p.drawEllipse(center, R, R);

        QRadialGradient textShadow(center, R);
        textShadow.setColorAt(0, QColor(20, 22, 25, 200));
        textShadow.setColorAt(1, QColor(20, 22, 25, 0));
        p.setPen(Qt::NoPen);
        p.setBrush(textShadow);
        p.drawEllipse(center, R, R);

        return;
    }

    // ── ACTIVE MAP ────────────────────────────────
    p.fillRect(rect(), QColor(15, 18, 25, 255));

    if (!mapBackground.isNull()) {
        double tW = mapBackground.width()  * zoom;
        double tH = mapBackground.height() * zoom;

        double ox = std::fmod(panX, tW);
        if (ox > 0) ox -= tW;
        double oy = std::fmod(panY, tH);
        if (oy > 0) oy -= tH;

        p.save();
        p.setOpacity(0.5);
        for (double x = ox; x < width()  + tW; x += tW)
            for (double y = oy; y < height() + tH; y += tH)
                p.drawPixmap(QRectF(x, y, tW, tH), mapBackground, mapBackground.rect());
        p.restore();
    }

    p.setPen(QPen(QColor(74, 144, 226, 30), 1));
    int gs = qMax(20, (int)(100 * zoom));
    int sx = ((int)panX % gs + gs) % gs;
    int sy = ((int)panY % gs + gs) % gs;
    for (int x = sx - gs; x < width()  + gs; x += gs) p.drawLine(x, 0, x, height());
    for (int y = sy - gs; y < height() + gs; y += gs) p.drawLine(0, y, width(), y);

    for (double x = std::fmod(panX, mapBackground.width() * zoom) - mapBackground.width() * zoom; x < width(); x += mapBackground.width() * zoom) {
        for (double y = std::fmod(panY, mapBackground.height() * zoom) - mapBackground.height() * zoom; y < height(); y += mapBackground.height() * zoom) {

            QPointF prev; bool hasPrev = false;
            for (const auto &node : nodes) {
                QPointF sp(x + node.x_frac * mapBackground.width() * zoom, y + node.y_frac * mapBackground.height() * zoom);

                if (hasPrev) {
                    p.setPen(QPen(QColor(74, 144, 226, 80), 2.0, Qt::DashLine));
                    p.drawLine(prev, sp);
                }

                bool isSelected = (node.title == selectedNodeTitle);
                QColor targetColor = isSelected ? QColor(74, 144, 226) : node.color;

                if (pulseCounter < 30) {
                    int r = pulseCounter;
                    p.setBrush(Qt::NoBrush);
                    p.setPen(QPen(QColor(targetColor.red(), targetColor.green(), targetColor.blue(), qMax(0, 200 - r * 6)), 2.0));
                    p.drawEllipse(sp, 6.0 + r, 6.0 + r);
                }

                double baseDot = qMax(4.0, 6.0 * zoom);
                double dot = isSelected ? (baseDot * 1.5) : baseDot;

                p.setBrush(targetColor); p.setPen(Qt::NoPen);
                p.drawEllipse(sp, dot, dot);

                p.setPen(QColor(200, 220, 255));
                QFont f = p.font(); f.setPointSize(10); f.setBold(true); p.setFont(f);
                p.drawText(QPointF(sp.x() + dot + 6, sp.y() - dot), node.title);

                prev = sp; hasPrev = true;
            }
        }
    }
}

// ==============================================================================
// MAIN WINDOW MODULE
// ==============================================================================

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    helpBtnMain = nullptr;
    imageViewer = nullptr;
    rightPanel = nullptr;
    mapWidget = nullptr;
    inputField = nullptr;

    setWindowTitle("OSINT Intelligence");

    clockTimer = new QTimer(this);
    connect(clockTimer, &QTimer::timeout, this, &MainWindow::updateTime);
    clockTimer->start(1000);

    setupUI();
    applyStyles();
    updateTime();
    setUser("agent@osint.io");

    resize(1400, 820);
}

MainWindow::~MainWindow() {}

void MainWindow::setUser(const QString &login)
{
    QString name = login.contains("@") ? login.split("@").first().toUpper() : login.toUpper();
    setWindowTitle("OSINT Intelligence — " + name);

    if (userNameFullLbl) userNameFullLbl->setText(name);
    if (userRoleFullLbl) userRoleFullLbl->setText(QString("SAI0044 · ") + (name == "ADMIN" ? "SYSTEM" : "AUTHORISED PERSONNEL"));
    if (userAvatarFullLbl) userAvatarFullLbl->setText(name.left(1));
    if (userAvatarMiniLbl) userAvatarMiniLbl->setText(name.left(1));
}

void MainWindow::onLogoutClicked()
{
    QString sessionFile = QCoreApplication::applicationDirPath() + "/session.json";
    QFile f(sessionFile);
    if (f.exists()) f.remove();
    QApplication::exit(1);
}

QWidget* MainWindow::createSectionDivider(const QString &title) {
    QWidget *w = new QWidget(); w->setObjectName("sectionDivider");
    QHBoxLayout *hl = new QHBoxLayout(w);
    hl->setContentsMargins(2, 10, 2, 4); hl->setSpacing(8);
    QLabel *lbl = new QLabel(title); lbl->setObjectName("sectionLabel");
    QFrame *line = new QFrame(); line->setFrameShape(QFrame::HLine);
    line->setObjectName("sectionLine");
    line->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    hl->addWidget(lbl); hl->addWidget(line, 1);
    return w;
}

void MainWindow::setupUI() {
    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    QHBoxLayout *ml = new QHBoxLayout(central);
    ml->setContentsMargins(0,0,0,0); ml->setSpacing(0);

    leftStack = new QStackedWidget();
    leftStack->setObjectName("leftStack");
    leftStack->setFixedWidth(LEFT_EXPANDED);

    QFrame *lFull = new QFrame(); lFull->setObjectName("leftPanel");
    QFrame *lMini = new QFrame(); lMini->setObjectName("leftPanel");
    setupLeftFull(lFull);
    setupLeftMini(lMini);
    leftStack->addWidget(lFull);
    leftStack->addWidget(lMini);

    QFrame *center = new QFrame(); center->setObjectName("centerPanel");
    setupCenterPanel(center);

    rightPanel = new QFrame(); rightPanel->setObjectName("rightPanel");
    rightPanel->setFixedWidth(280);
    setupRightPanel(rightPanel);
    rightPanel->setVisible(false);

    ml->addWidget(leftStack);
    ml->addWidget(center, 1);
    ml->addWidget(rightPanel);

    QString helpText = "Supported inputs:\n• Email address\n• Username / Bio\n• First/Last name\n• IP / Phone\n• Photo (EXIF)";
    helpBtnMain = makeIconBtn(":/icons/icon-help.png", "?", 42, 42, "roundBtn", helpText);
    helpBtnMain->setParent(central);
    helpBtnMain->setCursor(Qt::PointingHandCursor);
    connect(helpBtnMain, &QPushButton::clicked, this, &MainWindow::showHelpTooltip);
    helpBtnMain->raise();

    imageViewer = new InteractiveImageViewer(central);
    imageViewer->hide();
    connect(imageViewer, &InteractiveImageViewer::closed, this, &MainWindow::hideOverlayImage);

    QShortcut *escShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(escShortcut, &QShortcut::activated, this, &MainWindow::hideOverlayImage);

    connect(historyList, &QListWidget::itemClicked, this, &MainWindow::onCaseClicked);
}

void MainWindow::showHelpTooltip() {
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (btn) QToolTip::showText(btn->mapToGlobal(QPoint(btn->width(), -10)), btn->toolTip(), btn);
}

void MainWindow::setupLeftFull(QFrame *panel) {
    QVBoxLayout *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(14, 18, 14, 14); layout->setSpacing(4);

    QHBoxLayout *topRow = new QHBoxLayout();
    QPushButton *appNameBtn = new QPushButton("OSINT AI");
    appNameBtn->setObjectName("appNameBtn"); appNameBtn->setCursor(Qt::PointingHandCursor);
    connect(appNameBtn, &QPushButton::clicked, this, &MainWindow::goHome);
    QPushButton *collapseBtn = makeIconBtn(":/icons/icon-collapse.png", "◀", 28, 28, "collapseBtn", "Hide sidebar");
    connect(collapseBtn, &QPushButton::clicked, this, &MainWindow::toggleLeftPanel);
    topRow->addWidget(appNameBtn); topRow->addStretch(); topRow->addWidget(collapseBtn);
    layout->addLayout(topRow); layout->addSpacing(14);

    QLineEdit *searchBar = new QLineEdit();
    searchBar->setPlaceholderText("🔍  Search cases...");
    searchBar->setObjectName("sideSearch");
    layout->addWidget(searchBar); layout->addSpacing(6);

    historyList = new QListWidget();
    historyList->setObjectName("historyList"); historyList->setSpacing(3);

    auto addHist = [&](const QString &title, const QString &sub) {
        QListWidgetItem *item = new QListWidgetItem(historyList);
        QWidget *card = new QWidget(); card->setObjectName("histCard");
        QVBoxLayout *cl = new QVBoxLayout(card);
        cl->setContentsMargins(12,7,12,7); cl->setSpacing(2);
        QLabel *t = new QLabel(title); t->setObjectName("histTitle");
        QLabel *s = new QLabel(sub);   s->setObjectName("histSub");
        cl->addWidget(t); cl->addWidget(s);
        item->setSizeHint(QSize(0, 52));
        historyList->setItemWidget(item, card);
    };

    layout->addWidget(createSectionDivider("Today"));
    addHist("SAI0044 — Data Leak",    "Active");
    addHist("Profile Scan CRM",       "Target analysis");
    addHist("Alpha-9 Network Scan",   "Infrastructure map");
    layout->addWidget(createSectionDivider("Yesterday"));
    addHist("John Doe — OSINT trace", "Completed");
    addHist("IP 185.220.x.x analysis","Network mapping");
    layout->addWidget(historyList, 1);
    layout->addStretch();

    QLabel *disclaimer = new QLabel("DISCLAIMER: Use authorized only.\nAll actions monitored.\nProceed with caution.\nSystem Integrity Maintained.");
    disclaimer->setObjectName("disclaimerText");
    layout->addWidget(disclaimer); layout->addSpacing(8);

    QFrame *userFrame = new QFrame(); userFrame->setObjectName("userFrame");
    QHBoxLayout *ul = new QHBoxLayout(userFrame);
    ul->setContentsMargins(10,10,10,10); ul->setSpacing(10);
    userAvatarFullLbl = new QLabel("S"); userAvatarFullLbl->setObjectName("userAvatar");
    userAvatarFullLbl->setFixedSize(36,36);
    userAvatarFullLbl->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    QVBoxLayout *ui2 = new QVBoxLayout(); ui2->setSpacing(2);
    userNameFullLbl = new QLabel("USER"); userNameFullLbl->setObjectName("userName");
    userRoleFullLbl = new QLabel("ROLE"); userRoleFullLbl->setObjectName("userRole");
    ui2->addWidget(userNameFullLbl); ui2->addWidget(userRoleFullLbl);
    ul->addWidget(userAvatarFullLbl); ul->addLayout(ui2); ul->addStretch();
    layout->addWidget(userFrame); layout->addSpacing(6);

    QHBoxLayout *actRow = new QHBoxLayout(); actRow->setSpacing(6);
    QPushButton *settBtn = new QPushButton("⚙  Settings");
    connect(settBtn, &QPushButton::clicked, this, [this]() {
        SettingsDialog dlg(this);
        dlg.exec();
    });
    settBtn->setObjectName("settingsBtn"); settBtn->setCursor(Qt::PointingHandCursor);
    QPushButton *logoutBtn = makeIconBtn(":/icons/icon-logout.png", "→", 36, 36, "logoutBtn", "Sign out");
    logoutBtn->setCursor(Qt::PointingHandCursor);
    connect(logoutBtn, &QPushButton::clicked, this, &MainWindow::onLogoutClicked);
    actRow->addWidget(settBtn, 1); actRow->addWidget(logoutBtn);
    layout->addLayout(actRow);
}

void MainWindow::setupLeftMini(QFrame *panel) {
    QVBoxLayout *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(0, 18, 0, 14); layout->setSpacing(15);
    layout->setAlignment(Qt::AlignHCenter);

    QPushButton *expandBtn = makeIconBtn(":/icons/icon-expand.png", "▶", 34, 34, "collapseBtn", "Show sidebar");
    connect(expandBtn, &QPushButton::clicked, this, &MainWindow::toggleLeftPanel);
    layout->addWidget(expandBtn, 0, Qt::AlignHCenter); layout->addStretch();

    userAvatarMiniLbl = new QLabel("S"); userAvatarMiniLbl->setObjectName("userAvatar");
    userAvatarMiniLbl->setFixedSize(36,36);
    userAvatarMiniLbl->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    layout->addWidget(userAvatarMiniLbl, 0, Qt::AlignHCenter); layout->addSpacing(6);

    QPushButton *miniSettings = new QPushButton("⚙");
    miniSettings->setObjectName("miniRoundBtn"); miniSettings->setFixedSize(36, 36);
    QPushButton *miniLogout = makeIconBtn(":/icons/icon-logout.png", "→", 36, 36, "miniRoundBtn", "Sign out");
    connect(miniLogout, &QPushButton::clicked, this, &MainWindow::onLogoutClicked);
    layout->addWidget(miniSettings, 0, Qt::AlignHCenter);
    layout->addSpacing(4);
    layout->addWidget(miniLogout, 0, Qt::AlignHCenter);
}

void MainWindow::setupCenterPanel(QFrame *panel) {
    QGridLayout *grid = new QGridLayout(panel);
    grid->setContentsMargins(0,0,0,0);
    grid->setSpacing(0);

    mapWidget = new OSINTNodeMap();
    mapWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mapWidget->setMinimumSize(800, 600);
    grid->addWidget(mapWidget, 0, 0);

    QVBoxLayout *ol = new QVBoxLayout(mapWidget);
    ol->setContentsMargins(0, 30, 0, 40);
    ol->setSpacing(0);

    QFrame *timeFrame = new QFrame();
    timeFrame->setObjectName("timeFrame");
    QVBoxLayout *tl = new QVBoxLayout(timeFrame);
    tl->setContentsMargins(0,0,0,0); tl->setSpacing(0);
    timeLbl = new QLabel(); timeLbl->setObjectName("timeLbl"); timeLbl->setAlignment(Qt::AlignCenter);
    dateLbl = new QLabel(); dateLbl->setObjectName("dateLbl"); dateLbl->setAlignment(Qt::AlignCenter);
    tl->addWidget(timeLbl); tl->addWidget(dateLbl);
    ol->addWidget(timeFrame, 0, Qt::AlignTop | Qt::AlignHCenter);

    topSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
    ol->addSpacerItem(topSpacer);

    titleFrame = new QFrame();
    titleFrame->setObjectName("titleFrame");
    QVBoxLayout *cl = new QVBoxLayout(titleFrame);
    cl->setContentsMargins(0,0,0,0); cl->setSpacing(10);
    welcomeTitle = new QLabel("OSINT Intelligence"); welcomeTitle->setObjectName("welcomeTitle"); welcomeTitle->setAlignment(Qt::AlignCenter);
    welcomeSub = new QLabel("Enter a target — email, phone, username, BIO\nor upload a photo for metadata analysis"); welcomeSub->setObjectName("welcomeSub"); welcomeSub->setAlignment(Qt::AlignCenter);
    cl->addWidget(welcomeTitle); cl->addWidget(welcomeSub);
    ol->addWidget(titleFrame, 0, Qt::AlignHCenter);

    ol->addSpacing(30);

    QFrame *chatBar = new QFrame();
    chatBar->setObjectName("chatBar");
    chatBar->setStyleSheet("background: transparent;");
    QHBoxLayout *chatRow = new QHBoxLayout(chatBar);
    chatRow->setContentsMargins(0,0,0,0); chatRow->setSpacing(10);

    inputBox = new QFrame(); inputBox->setObjectName("inputBox");
    inputBox->setFixedHeight(56);
    inputBox->setMinimumWidth(600);

    inputBox->setStyleSheet("QFrame#inputBox { background-color: #1A1D21; border: 1.5px solid #3C4043; border-radius: 28px; }");

    QHBoxLayout *ibl = new QHBoxLayout(inputBox);
    ibl->setContentsMargins(14,0,10,0); ibl->setSpacing(8);

    QPushButton *attachBtn = new QPushButton("+");
    attachBtn->setObjectName("attachBtn"); attachBtn->setFixedSize(32,32);
    attachBtn->setCursor(Qt::PointingHandCursor);

    inputField = new QLineEdit();
    inputField->setObjectName("chatInput");
    inputField->setPlaceholderText("Enter target: email, BIO, phone, IP or upload photo...");
    inputField->installEventFilter(this);
    connect(inputField, &QLineEdit::returnPressed, this, &MainWindow::sendMessage);

    QPushButton *sendBtn = makeIconBtn(":/icons/icon-send.png", "↑", 38, 38, "sendBtn", "Send");
    sendBtn->setCursor(Qt::PointingHandCursor);
    connect(sendBtn, &QPushButton::clicked, this, &MainWindow::sendMessage);

    ibl->addWidget(attachBtn, 0, Qt::AlignVCenter);
    ibl->addWidget(inputField, 1, Qt::AlignVCenter);
    ibl->addWidget(sendBtn,   0, Qt::AlignVCenter);
    chatRow->addWidget(inputBox, 1);

    QString helpText = "Supported inputs:\n• Email address\n• Username / Bio\n• First name / Last name\n• IP address or Phone number\n• Photo (EXIF / GEO metadata)";
    helpBtnChat = makeIconBtn(":/icons/icon-help.png", "?", 42, 42, "roundBtn", helpText);
    helpBtnChat->setVisible(false);
    helpBtnChat->setCursor(Qt::PointingHandCursor);
    connect(helpBtnChat, &QPushButton::clicked, this, &MainWindow::showHelpTooltip);
    chatRow->addWidget(helpBtnChat, 0, Qt::AlignVCenter);

    ol->addWidget(chatBar, 0, Qt::AlignHCenter);

    QLabel *disc = new QLabel("OSINT AI may produce inaccurate information. Always verify results independently.");
    disc->setObjectName("chatDisclaimer"); disc->setAlignment(Qt::AlignCenter);
    ol->addWidget(disc, 0, Qt::AlignHCenter);

    bottomSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
    ol->addSpacerItem(bottomSpacer);

    connect(mapWidget, &OSINTNodeMap::nodeClicked, this, &MainWindow::updateRightPanel);
}

void MainWindow::setupRightPanel(QFrame *panel) {
    QVBoxLayout *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(14,18,14,14); layout->setSpacing(10);

    QFrame *tf = new QFrame(); tf->setObjectName("toggleFrame");
    QHBoxLayout *tgl = new QHBoxLayout(tf);
    tgl->setContentsMargins(4,4,4,4); tgl->setSpacing(2);
    QPushButton *b2D = new QPushButton("2D"); b2D->setObjectName("toggleActive");
    QPushButton *b3D = new QPushButton("3D"); b3D->setObjectName("toggleInactive");
    tgl->addWidget(b2D); tgl->addWidget(b3D);
    layout->addWidget(tf);

    QFrame *infoCard = new QFrame(); infoCard->setObjectName("infoCard");
    QVBoxLayout *icl = new QVBoxLayout(infoCard);
    icl->setContentsMargins(16,14,16,14); icl->setSpacing(6);

    cardTitle = new QLabel("TARGET NODE"); cardTitle->setObjectName("cardTitle");
    cardData = new QLabel("Select a node on the map to view metadata.");
    cardData->setObjectName("cardData");
    cardData->setTextFormat(Qt::RichText);
    cardData->setWordWrap(true);
    cardData->setOpenExternalLinks(false);
    connect(cardData, &QLabel::linkActivated, this, &MainWindow::handleLinkClick);

    icl->addWidget(cardTitle);
    icl->addWidget(cardData);
    layout->addWidget(infoCard);
    layout->addStretch();
}

void MainWindow::toggleLeftPanel() {
    leftExpanded = !leftExpanded;
    leftStack->setCurrentIndex(leftExpanded ? 0 : 1);
    leftStack->setFixedWidth(leftExpanded ? LEFT_EXPANDED : LEFT_COLLAPSED);
}

void MainWindow::onCaseClicked() {
    rightPanel->hide();
    helpBtnMain->setVisible(false);
    helpBtnChat->setVisible(true);

    titleFrame->hide();
    bottomSpacer->changeSize(0, 0, QSizePolicy::Minimum, QSizePolicy::Fixed);
    mapWidget->layout()->invalidate();

    mapWidget->setMode(true);
    mapWidget->generateRandomCase();
}

void MainWindow::goHome() {
    inputField->clear();
    rightPanel->hide();
    helpBtnMain->setVisible(true);
    helpBtnChat->setVisible(false);

    titleFrame->show();
    bottomSpacer->changeSize(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
    mapWidget->layout()->invalidate();

    mapWidget->setMode(false);
}

void MainWindow::updateRightPanel(QString title, QString info) {
    cardTitle->setText(title.toUpper());
    cardData->setText(info);
    rightPanel->show();
}

void MainWindow::updateTime() {
    QDateTime now = QDateTime::currentDateTime();
    timeLbl->setText(now.toString("HH:mm"));
    dateLbl->setText(now.toString("dddd, d MMM yyyy").toUpper());
}

void MainWindow::sendMessage() {
    QString text = inputField->text().trimmed();
    if (text.isEmpty()) return;
    pendingMsg = text;
    inputField->clear();
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if (obj == inputField) {
        if (event->type() == QEvent::FocusIn) {
            inputBox->setStyleSheet(
                "QFrame#inputBox { background-color: #111D33;"
                " border: 2px solid #4A90E2; border-radius: 28px; }"
                );
        } else if (event->type() == QEvent::FocusOut) {
            inputBox->setStyleSheet("QFrame#inputBox { background-color: #1A1D21; border: 1.5px solid #3C4043; border-radius: 28px; }");
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::handleLinkClick(const QString &link) {
    if (link.startsWith("img://")) {
        QString path = link.mid(6);
        QPixmap pixmap(path);

        if (!pixmap.isNull()) {
            imageViewer->resize(centralWidget()->size());
            imageViewer->setImage(pixmap);
            imageViewer->show();
            imageViewer->raise();
        }
    }
}

void MainWindow::hideOverlayImage() {
    if (imageViewer) imageViewer->hide();
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);

    if (helpBtnMain && centralWidget()) {
        helpBtnMain->move(
            centralWidget()->width()  - helpBtnMain->width()  - 20,
            centralWidget()->height() - helpBtnMain->height() - 20
            );
        helpBtnMain->raise();
    }

    if (imageViewer != nullptr && !imageViewer->isHidden()) {
        imageViewer->resize(centralWidget()->size());
    }
}

void MainWindow::applyStyles() {
    qApp->setStyleSheet(R"(
QMainWindow, QWidget { background-color: #1E1F20; color: #E3E3E3;
    font-family: "Segoe UI","Inter",Arial,sans-serif; font-size: 15px; }

QFrame#leftPanel, QStackedWidget#leftStack { background-color: #131314; }

QPushButton#appNameBtn { background: transparent; border: none;
    color: #E3E3E3; font-size: 22px; font-weight: 800; text-align: left; padding: 4px 2px; }
QPushButton#appNameBtn:hover { color: #8AB4F8; }

QPushButton#collapseBtn { background: transparent; border: 1px solid #3C4043;
    border-radius: 14px; color: #9AA0A6; font-size: 11px; padding: 0; }
QPushButton#collapseBtn:hover { background-color: #282A2C; color: #E3E3E3; }

QLineEdit#sideSearch { background-color: #282A2C; border: 1.5px solid #3C4043;
    border-radius: 20px; padding: 8px 16px; color: #E3E3E3; font-size: 14px; }
QLineEdit#sideSearch:focus { border: 2px solid #4A90E2; background-color: #111D33; }

QWidget#sectionDivider { background-color: #131314; }
QLabel#sectionLabel { color: #9AA0A6; font-size: 11px; font-weight: 700; letter-spacing: 1px; background: transparent; }
QFrame#sectionLine { color: #2E3033; background-color: #2E3033; max-height: 1px; border: none; }

QListWidget#historyList { background: transparent; border: none; }
QListWidget#historyList::item { background: transparent; border-radius: 10px; }
QListWidget#historyList::item:hover,
QListWidget#historyList::item:selected { background-color: #282A2C; border-radius: 10px; }
QWidget#histCard { background-color: transparent; border-radius: 10px; }
QLabel#histTitle { color: #C4C7C5; font-size: 14px; background: transparent; }
QLabel#histSub   { color: #9AA0A6; font-size: 12px; background: transparent; }

QLabel#disclaimerText { color: #5F6368; font-size: 11px; background: transparent; }

QFrame#userFrame { background-color: #282A2C; border-radius: 12px; border: 1px solid #3C4043; }
QLabel#userAvatar { background-color: #8AB4F8; color: #131314; border-radius: 18px;
    font-weight: 700; font-size: 14px; min-width:36px; max-width:36px;
    min-height:36px; max-height:36px; padding: 0; margin: 0; text-align: center; }
QLabel#userName { color: #E3E3E3; font-size: 14px; font-weight: 600; background: transparent; }
QLabel#userRole { color: #9AA0A6; font-size: 12px; background: transparent; }

QPushButton#settingsBtn { background-color: #282A2C; color: #9AA0A6;
    border: 1px solid #3C4043; border-radius: 10px; font-size: 13px;
    padding: 8px 12px; text-align: left; }
QPushButton#settingsBtn:hover { background-color: #35363A; color: #E3E3E3; }

QPushButton#logoutBtn, QPushButton#miniRoundBtn {
    background-color: #282A2C; color: #9AA0A6;
    border: 1px solid #3C4043; border-radius: 18px; font-size: 16px;
    padding: 0; font-weight: 700; text-align: center;
}
QPushButton#logoutBtn:hover, QPushButton#miniRoundBtn:hover {
    background-color: #3D2020; border: 1px solid #FF6B6B;
}

QFrame#centerPanel { background-color: #1E1F20; }
QFrame#centerOverlay, QFrame#chatBar { background: transparent; }

QLabel#timeLbl { color: #E3E3E3; font-size: 48px; font-weight: 300;
    background: transparent; letter-spacing: 2px; }
QLabel#dateLbl { color: #9AA0A6; font-size: 14px; background: transparent; letter-spacing: 1px; }
QLabel#welcomeTitle { color: #E3E3E3; font-size: 34px; font-weight: 700; background: transparent; }
QLabel#welcomeSub   { color: #9AA0A6; font-size: 15px; background: transparent; }

QFrame#inputBox {
    background-color: #1A1D21;
    border: 1.5px solid #3C4043;
    border-radius: 28px;
}
QLineEdit#chatInput { background-color: transparent; border: none; color: #E3E3E3; font-size: 15px; }

QPushButton#attachBtn {
    background: transparent; border: none; color: #9AA0A6;
    font-size: 22px; font-weight: 300; border-radius: 16px; padding: 0; margin: 0;
    text-align: center;
}
QPushButton#attachBtn:hover { background-color: #35363A; color: #E3E3E3; }

QPushButton#sendBtn {
    background-color: #4A90E2; color: #FFFFFF; border: none;
    border-radius: 19px; font-size: 16px; font-weight: 700; padding: 0;
    text-align: center;
}
QPushButton#sendBtn:hover { background-color: #5BA3F5; }

QFrame#timeFrame, QFrame#titleFrame { background: transparent; border: none; }
QLabel#chatDisclaimer { color: #FFFFFF; background: transparent; }

QPushButton#roundBtn {
    background-color: #1A1D21; color: #9AA0A6;
    border: 1.5px solid #3C4043; border-radius: 21px; font-weight: 700;
    font-size: 16px; padding: 0; text-align: center;
}
QPushButton#roundBtn:hover { background-color: #35363A; color: #E3E3E3; }

QToolTip { color: #E3E3E3; background-color: #282A2C; border: 1px solid #4A90E2;
    border-radius: 4px; padding: 6px; font-size: 13px; }

QFrame#rightPanel { background-color: #131314; border-left: 1px solid #3C4043; }
QFrame#toggleFrame { background-color: #282A2C; border-radius: 8px; }
QPushButton#toggleActive { background-color: #35363A; color: #E3E3E3; border: none;
    border-radius: 6px; padding: 7px 20px; font-size: 14px; font-weight: 600; }
QPushButton#toggleInactive { background: transparent; color: #9AA0A6; border: none;
    padding: 7px 20px; font-size: 14px; }
QPushButton#toggleInactive:hover { color: #E3E3E3; }
QLabel#mapPlaceholder { background-color: #282A2C; border-radius: 14px; color: #5F6368; font-size: 16px; }
QFrame#infoCard { background-color: #282A2C; border-radius: 14px; }
QLabel#cardTitle { color: #4A90E2; font-size: 12px; font-weight: 700; letter-spacing: 1px; background: transparent; }
QLabel#cardData  { color: #E3E3E3; font-size: 14px; background: transparent; }

QScrollBar:vertical { background: transparent; width: 6px; }
QScrollBar::handle:vertical { background: #3C4043; border-radius: 3px; min-height: 20px; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
    )");
}