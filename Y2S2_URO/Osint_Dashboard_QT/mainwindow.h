#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>
#include <QFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPainter>
#include <QDateTime>
#include <QPixmap>
#include <QToolTip>
#include <QPainterPath>
#include <QShortcut>
#include <QSpacerItem>

// === DATA STRUCTURES ===
struct OsintNode
{
    double x_frac;
    double y_frac;
    QString title;
    QString info;
    QColor color;
};

struct BackgroundParticle
{
    double x, y, speed, size;
    int alpha;
};

// === INTERACTIVE IMAGE VIEWER MODULE ===
class InteractiveImageViewer : public QWidget
{
    Q_OBJECT

public:
    explicit InteractiveImageViewer(QWidget *parent = nullptr);
    void setImage(const QPixmap &pixmap);

signals:
    void closed();

protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QPixmap image;
    double zoom;
    double panX, panY;
    QPoint lastMousePos;
    QPushButton *closeBtn;
};

// === OSINT NODE MAP MODULE ===
class OSINTNodeMap : public QWidget
{
    Q_OBJECT

public:
    explicit OSINTNodeMap(QWidget *parent = nullptr);
    void setMode(bool active);
    void generateRandomCase();

signals:
    void nodeClicked(QString title, QString info);

protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void initParticles();
    bool isActive;
    double panX, panY;
    double zoom;

    QPoint lastMousePos;
    QTimer *pulseTimer;

    double rotationAngle;
    int pulseCounter;

    QVector<OsintNode> nodes;
    QVector<BackgroundParticle> bgParticles;
    QPixmap mapBackground;
    QString selectedNodeTitle;
};

// === MAIN WINDOW MODULE ===
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void setUser(const QString &login);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void toggleLeftPanel();
    void onCaseClicked();

    void goHome();
    void sendMessage();

    void updateRightPanel(QString title, QString info);
    void updateTime();

    void onLogoutClicked();
    void showHelpTooltip();

    void handleLinkClick(const QString &link);
    void hideOverlayImage();

private:
    void setupUI();
    void setupLeftFull(QFrame *panel);
    void setupLeftMini(QFrame *panel);
    void setupCenterPanel(QFrame *panel);
    void setupRightPanel(QFrame *panel);

    void applyStyles();
    QWidget* createSectionDivider(const QString &title);

    QStackedWidget *leftStack;
    QListWidget *historyList;
    QFrame *rightPanel;
    QFrame *inputBox;
    QLineEdit *inputField;
    QString pendingMsg;

    QLabel *timeLbl;
    QLabel *dateLbl;
    QTimer *clockTimer;

    QPushButton *helpBtnMain;
    QPushButton *helpBtnChat;

    // === Layout Animation Elements ===
    QFrame *titleFrame;
    QLabel *welcomeTitle;
    QLabel *welcomeSub;
    QSpacerItem *topSpacer;
    QSpacerItem *bottomSpacer;

    OSINTNodeMap *mapWidget;
    QLabel *cardTitle;
    QLabel *cardData;

    QLabel *userNameFullLbl = nullptr;
    QLabel *userRoleFullLbl = nullptr;
    QLabel *userAvatarFullLbl = nullptr;
    QLabel *userAvatarMiniLbl = nullptr;

    QLabel *imageOverlay;
    InteractiveImageViewer *imageViewer;

    bool leftExpanded = true;
    const int LEFT_EXPANDED = 260;
    const int LEFT_COLLAPSED = 75;
};

#endif // MAINWINDOW_H