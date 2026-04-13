#include "mainwindow.h"
#include "logindialog.h"
#include <QApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCoreApplication>
#include <QFontDatabase>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    int fontId = QFontDatabase::addApplicationFont(":/fonts/Comfortaa-VariableFont_wght.ttf");
    if (fontId != -1)
    {
        QString fontFamily = QFontDatabase::applicationFontFamilies(fontId).at(0);
        QApplication::setFont(QFont(fontFamily, 10));
    }

    int exitCode = 0;

    do
    {
        // Check saved session
        QString sessionFile = QCoreApplication::applicationDirPath() + "/session.json";
        QString autoEmail;
        QFile sf(sessionFile);
        if (sf.open(QIODevice::ReadOnly))
        {
            QJsonDocument doc = QJsonDocument::fromJson(sf.readAll());
            sf.close();
            autoEmail = doc.object()["email"].toString();
        }

        QString loginEmail;
        if (autoEmail.isEmpty())
        {
            LoginDialog dlg;

            if (dlg.exec() != QDialog::Accepted)
                return 0;

            loginEmail = dlg.loggedInEmail;
        }
        else
        {
            loginEmail = autoEmail;
        }

        MainWindow w;
        w.setUser(loginEmail);
        w.show();
        exitCode = a.exec(); // 1 = logout, 0 = standard exit
    }
    while (exitCode == 1);

    return 0;
}