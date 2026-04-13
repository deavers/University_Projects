#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QTimer>
#include <QVector>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QMap>
#include <QAction>
#include <QLabel>
#include <QPropertyAnimation>

struct Particle
{
    float x, y;
    float vx, vy;
    float radius;
    int alpha;
};

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();

    QString loggedInEmail;

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateParticles();
    void onLoginClicked();
    void onGithubClicked();
    void onForgotClicked();
    void togglePasswordVisibility();
    void clearError();

private:
    void setupUI();
    void initParticles();

    void saveSession(const QString &email, const QString &username);

    void shakeCard();

    void setError(const QString &msg);
    void setSuccess(const QString &msg);

    QTimer *animationTimer;

    QFrame *card;
    QLabel *errorLabel;
    QLineEdit *emailInput;
    QLineEdit *passwordInput;
    QAction *togglePassAction;
    QCheckBox *rememberMeCheck;
    QPushButton *forgotPassBtn;
    QPushButton *loginBtn;
    QPushButton *githubBtn;

    QMap<QString, QString> validUsers;
};

#endif // LOGINDIALOG_H