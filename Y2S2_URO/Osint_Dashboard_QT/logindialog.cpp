#include "logindialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QStyle>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QEvent>
#include <QPainter>

// ==============================================================================
// BACKGROUND PARTICLE CLASS
// ==============================================================================
class ParticleBackground : public QWidget
{
public:
    QVector<Particle> particles;
    ParticleBackground(QWidget *parent = nullptr) : QWidget(parent)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setPen(Qt::NoPen);
        for (const Particle &p : particles)
        {
            painter.setBrush(QColor(255, 255, 255, p.alpha));
            painter.drawRect(p.x, p.y, p.radius * 2, p.radius * 2);
        }
    }
};

ParticleBackground *bgWidget;

// ==============================================================================
// MAIN DIALOG CLASS
// ==============================================================================

LoginDialog::LoginDialog(QWidget *parent) : QDialog(parent)
{
    resize(1280, 800);
    setWindowTitle("OSINT Portal AI - Login");
    setStyleSheet("QDialog { background-color: #0D0F12; }");

    validUsers["admin"] = "admin";
    validUsers["agent@osint.io"] = "deaveAI";

    bgWidget = new ParticleBackground(this);
    bgWidget->resize(this->size());

    setupUI();
    initParticles();

    animationTimer = new QTimer(this);
    connect(animationTimer, &QTimer::timeout, this, &LoginDialog::updateParticles);
    animationTimer->start(30);
}

LoginDialog::~LoginDialog() {}

void LoginDialog::setupUI()
{
    card = new QFrame(this);
    card->setObjectName("loginCard");
    card->setFixedSize(400, 540);

    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(35, 30, 35, 30);
    cardLayout->setSpacing(10);

    QLabel *title = new QLabel("Welcome to OSINT AI", card);
    title->setObjectName("title");
    title->setAlignment(Qt::AlignCenter);

    QLabel *subtitle = new QLabel("Advanced open-source intelligence platform.\nAnalyze targets, gather evidence, and map networks.", card);
    subtitle->setObjectName("subtitle");
    subtitle->setAlignment(Qt::AlignCenter);
    subtitle->setWordWrap(true);

    errorLabel = new QLabel("", card);
    errorLabel->setObjectName("errorLabel");
    errorLabel->setAlignment(Qt::AlignCenter);
    errorLabel->setFixedHeight(18);

    QLabel *emailLabel = new QLabel("Email Address", card);
    emailLabel->setObjectName("inputLabel");
    emailInput = new QLineEdit(card);
    emailInput->setPlaceholderText("manager@osint.io");

    QLabel *passLabel = new QLabel("Password", card);
    passLabel->setObjectName("inputLabel");
    passwordInput = new QLineEdit(card);
    passwordInput->setPlaceholderText("••••••••");
    passwordInput->setEchoMode(QLineEdit::Password);

    togglePassAction = passwordInput->addAction(QIcon(":/icons/icon-eye_closed.png"), QLineEdit::TrailingPosition);
    connect(togglePassAction, &QAction::triggered, this, &LoginDialog::togglePasswordVisibility);

    connect(emailInput, &QLineEdit::textChanged, this, &LoginDialog::clearError);
    connect(passwordInput, &QLineEdit::textChanged, this, &LoginDialog::clearError);

    QHBoxLayout *optionsLayout = new QHBoxLayout();
    rememberMeCheck = new QCheckBox("Remember me", card);
    rememberMeCheck->setChecked(true);

    forgotPassBtn = new QPushButton("Forgot password?", card);
    forgotPassBtn->setObjectName("forgotPassBtn");
    forgotPassBtn->setCursor(Qt::PointingHandCursor);
    forgotPassBtn->setFocusPolicy(Qt::NoFocus);
    forgotPassBtn->setAutoDefault(false);
    connect(forgotPassBtn, &QPushButton::clicked, this, &LoginDialog::onForgotClicked);

    optionsLayout->addWidget(rememberMeCheck);
    optionsLayout->addStretch();
    optionsLayout->addWidget(forgotPassBtn);

    loginBtn = new QPushButton("Login to portal →", card);
    loginBtn->setObjectName("loginBtn");
    loginBtn->setMinimumHeight(45);
    loginBtn->setCursor(Qt::PointingHandCursor);
    loginBtn->setDefault(true);

    connect(loginBtn, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);

    githubBtn = new QPushButton(QIcon(":/icons/icon-github.png"), " Sign in with GitHub", card);
    githubBtn->setObjectName("githubBtn");
    githubBtn->setMinimumHeight(45);
    githubBtn->setCursor(Qt::PointingHandCursor);
    connect(githubBtn, &QPushButton::clicked, this, &LoginDialog::onGithubClicked);

    cardLayout->addWidget(title);
    cardLayout->addWidget(subtitle);
    cardLayout->addWidget(errorLabel);
    cardLayout->addSpacing(5);
    cardLayout->addWidget(emailLabel);
    cardLayout->addWidget(emailInput);
    cardLayout->addWidget(passLabel);
    cardLayout->addWidget(passwordInput);
    cardLayout->addLayout(optionsLayout);
    cardLayout->addSpacing(10);
    cardLayout->addWidget(loginBtn);
    cardLayout->addSpacing(10);
    cardLayout->addWidget(githubBtn);
    cardLayout->addStretch();

    // === QSS STYLING: TRANSPARENT INPUT BACKGROUND ===
    QString style = R"(
        #loginCard {
            background-color: rgba(22, 25, 30, 230);
            border: 1px solid rgba(255, 255, 255, 30);
            border-radius: 20px;
        }

        QLabel, QCheckBox { background: transparent; }

        #title { color: #FFFFFF; font-size: 24px; font-weight: bold; margin-bottom: 5px; }
        #subtitle { color: #8B949E; font-size: 12px; line-height: 1.4; }
        #errorLabel { font-size: 13px; font-weight: bold; min-height: 18px; }
        #inputLabel { color: #C9D1D9; font-size: 14px; }

        QLineEdit {
            background-color: transparent; /* <--- BLENDS WITH CARD NOW */
            border: 1px solid #30363D;
            border-radius: 6px;
            padding: 10px 40px 10px 10px;
            color: #FFFFFF;
            font-size: 15px;
            selection-background-color: #4A5568;
            selection-color: #FFFFFF;
            lineedit-password-character: 42;
        }
        QLineEdit:focus { border: 1px solid #58A6FF; background-color: rgba(255,255,255, 5); }
        QLineEdit[error="true"] { border: 1px solid #EF4444; }
        QLineEdit QToolButton { icon-size: 24px; padding: 0px 5px; background: transparent; }

        #loginBtn { background-color: #238636; color: #FFFFFF; border: none; border-radius: 6px; font-size: 15px; font-weight: bold; }
        #loginBtn:hover { background-color: #2EA043; }

        #githubBtn { background-color: #2F363D; color: #FFFFFF; border: 1px solid rgba(255, 255, 255, 30); border-radius: 6px; font-size: 15px; font-weight: bold; padding-left: 10px; }
        #githubBtn:hover { background-color: #3F464D; }

        QCheckBox { color: #8B949E; font-size: 13px; }
        #forgotPassBtn { background-color: transparent; border: none; color: #8B949E; font-size: 12px; text-decoration: underline; padding: 0; text-align: right; }
        #forgotPassBtn:hover { color: #58A6FF; }
    )";
    card->setStyleSheet(style);
}

// === ERROR HANDLING & SHAKE ANIMATION ===
void LoginDialog::setError(const QString &msg)
{
    errorLabel->setStyleSheet("color: #EF4444;"); // RED
    errorLabel->setText(msg);

    emailInput->setProperty("error", true);
    passwordInput->setProperty("error", true);

    emailInput->style()->unpolish(emailInput);
    emailInput->style()->polish(emailInput);
    passwordInput->style()->unpolish(passwordInput);
    passwordInput->style()->polish(passwordInput);

    QTimer::singleShot(20, this, &LoginDialog::shakeCard);
}

void LoginDialog::setSuccess(const QString &msg)
{
    errorLabel->setStyleSheet("color: #00FF41;"); // GREEN
    errorLabel->setText(msg);

    emailInput->setProperty("error", false);
    passwordInput->setProperty("error", false);

    emailInput->style()->unpolish(emailInput);
    emailInput->style()->polish(emailInput);
    passwordInput->style()->unpolish(passwordInput);
    passwordInput->style()->polish(passwordInput);
}

void LoginDialog::clearError()
{
    if (errorLabel->text().isEmpty()) return;
    errorLabel->setText("");
    emailInput->setProperty("error", false);
    passwordInput->setProperty("error", false);
    emailInput->style()->unpolish(emailInput);
    emailInput->style()->polish(emailInput);
    passwordInput->style()->unpolish(passwordInput);
    passwordInput->style()->polish(passwordInput);
}

void LoginDialog::shakeCard()
{
    animationTimer->stop();
    QPropertyAnimation *shake = new QPropertyAnimation(card, "pos");
    shake->setDuration(400);

    QPoint basePos((width() - card->width()) / 2, (height() - card->height()) / 2);

    shake->setKeyValueAt(0, basePos);
    shake->setKeyValueAt(0.1, basePos + QPoint(-10, 0));
    shake->setKeyValueAt(0.2, basePos + QPoint(10, 0));
    shake->setKeyValueAt(0.3, basePos + QPoint(-10, 0));
    shake->setKeyValueAt(0.5, basePos + QPoint(10, 0));
    shake->setKeyValueAt(0.7, basePos + QPoint(-5, 0));
    shake->setKeyValueAt(1.0, basePos);

    connect(shake, &QPropertyAnimation::finished, this, [this](){
        animationTimer->start(30);
    });

    shake->start(QAbstractAnimation::DeleteWhenStopped);
}

// === BUTTON HANDLERS ===
void LoginDialog::onLoginClicked()
{
    QString email = emailInput->text().trimmed();
    QString pwd = passwordInput->text();

    if (!validUsers.contains(email) || validUsers[email] != pwd)
    {
        setError("⚠ Invalid email or password.");
        return;
    }

    QString username = email.split("@")[0];
    username[0] = username[0].toUpper();

    if (rememberMeCheck->isChecked())
    {
        saveSession(email, username);
    }
    else
    {
        QFile file(QCoreApplication::applicationDirPath() + "/session.json");
        if (file.exists()) file.remove();
    }

    loggedInEmail = email;
    accept();
}

void LoginDialog::onGithubClicked()
{
    setError("⚠ GitHub SSO not configured yet.");
}

void LoginDialog::onForgotClicked()
{
    QString email = emailInput->text().trimmed();
    if (email.isEmpty())
    {
        setError("⚠ Enter your email to recover password.");
    }
    else
    {
        setSuccess("✔ Recovery code sent to " + email);
    }
}

void LoginDialog::togglePasswordVisibility()
{
    if (passwordInput->echoMode() == QLineEdit::Password)
    {
        passwordInput->setEchoMode(QLineEdit::Normal);
        togglePassAction->setIcon(QIcon(":/icons/icon-eye_opened.png"));
    }
    else
    {
        passwordInput->setEchoMode(QLineEdit::Password);
        togglePassAction->setIcon(QIcon(":/icons/icon-eye_closed.png"));
    }
}

// === ADAPTIVE PARTICLES ===
void LoginDialog::initParticles()
{
    bgWidget->particles.clear();
    QRandomGenerator *rg = QRandomGenerator::global();

    int w = width() > 100 ? width() : 1920;
    int h = height() > 100 ? height() : 1080;

    for (int i = 0; i < 150; ++i)
    {
        Particle p;
        p.x = rg->bounded(w);
        p.y = rg->bounded(h);

        p.vx = (rg->bounded(200) - 100) / 200.0f;
        p.vy = (rg->bounded(200) - 100) / 200.0f;

        p.radius = rg->bounded(1, 4);
        p.alpha = rg->bounded(50, 200);

        bgWidget->particles.append(p);
    }
}

void LoginDialog::resizeEvent(QResizeEvent *event)
{
    QDialog::resizeEvent(event);
    if (bgWidget)
        bgWidget->resize(this->size());

    if (card)
    {
        card->move((width() - card->width()) / 2, (height() - card->height()) / 2);
    }
    QRandomGenerator *rg = QRandomGenerator::global();

    for (int i = 0; i < bgWidget->particles.size(); ++i)
    {
        if (bgWidget->particles[i].x > width())
            bgWidget->particles[i].x = rg->bounded(width());

        if (bgWidget->particles[i].y > height())
            bgWidget->particles[i].y = rg->bounded(height());
    }
}

void LoginDialog::updateParticles()
{
    for (int i = 0; i < bgWidget->particles.size(); ++i)
    {
        bgWidget->particles[i].x += bgWidget->particles[i].vx;
        bgWidget->particles[i].y += bgWidget->particles[i].vy;

        if (bgWidget->particles[i].x < 0)
            bgWidget->particles[i].x = width();

        if (bgWidget->particles[i].x > width())
            bgWidget->particles[i].x = 0;

        if (bgWidget->particles[i].y < 0)
            bgWidget->particles[i].y = height();

        if (bgWidget->particles[i].y > height())
            bgWidget->particles[i].y = 0;
    }
    bgWidget->update();
}

void LoginDialog::saveSession(const QString &email, const QString &username)
{
    QJsonObject sessionData;
    sessionData["email"] = email;
    sessionData["username"] = username;
    sessionData["token"] = "token_" + email;

    QJsonDocument doc(sessionData);
    QString sessionFile = QCoreApplication::applicationDirPath() + "/session.json";
    QFile file(sessionFile);

    if (file.open(QIODevice::WriteOnly))
    {
        file.write(doc.toJson());
        file.close();
    }
}