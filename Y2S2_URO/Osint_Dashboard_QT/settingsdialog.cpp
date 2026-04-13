#include "settingsdialog.h"

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("SYSTEM CONFIGURATION");
    setFixedSize(400, 450);
    setupUI();
    applyStyles();
}

void SettingsDialog::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(25, 25, 25, 25);
    mainLayout->setSpacing(15);

    QLabel *header = new QLabel("NODE SETTINGS");
    header->setObjectName("header");
    mainLayout->addWidget(header);

    // --- Section: Network ---
    QFrame *line = new QFrame(); line->setFrameShape(QFrame::HLine); line->setStyleSheet("background: #3C4043;");
    mainLayout->addWidget(line);

    mainLayout->addWidget(new QLabel("Primary Uplink Server:"));
    QComboBox *serverBox = new QComboBox();
    serverBox->addItems({"EU-West (Frankfurt)", "US-East (Virginia)", "Asia-North (Tokyo)", "Local Proxy"});
    mainLayout->addWidget(serverBox);

    // --- Section: Security ---
    mainLayout->addSpacing(10);
    QCheckBox *encCheck = new QCheckBox("Enable End-to-End Encryption");
    encCheck->setChecked(true);
    mainLayout->addWidget(encCheck);

    QCheckBox *logCheck = new QCheckBox("Auto-wipe session logs on exit");
    mainLayout->addWidget(logCheck);

    QCheckBox *metaCheck = new QCheckBox("Hide IP via VPN routing");
    metaCheck->setChecked(true);
    mainLayout->addWidget(metaCheck);

    mainLayout->addStretch();

    // --- Buttons ---
    QHBoxLayout *btnRow = new QHBoxLayout();
    QPushButton *saveBtn = new QPushButton("APPLY");
    saveBtn->setCursor(Qt::PointingHandCursor);
    saveBtn->setFixedHeight(35);
    connect(saveBtn, &QPushButton::clicked, this, &QDialog::accept);

    QPushButton *cancelBtn = new QPushButton("CANCEL");
    cancelBtn->setObjectName("cancelBtn");
    cancelBtn->setCursor(Qt::PointingHandCursor);
    cancelBtn->setFixedHeight(35);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    btnRow->addWidget(cancelBtn);
    btnRow->addWidget(saveBtn);
    mainLayout->addLayout(btnRow);
}

void SettingsDialog::applyStyles() {
    setStyleSheet(R"(
        QDialog { background-color: #1E1F20; color: #E3E3E3; font-family: 'Segoe UI', sans-serif; }
        QLabel { color: #9AA0A6; font-size: 13px; }
        QLabel#header { color: #FFFFFF; font-size: 18px; font-weight: bold; margin-bottom: 5px; }

        QComboBox {
            background-color: #282A2C; border: 1px solid #3C4043; border-radius: 6px;
            padding: 5px 10px; color: #FFFFFF;
        }
        QComboBox::drop-down { border: none; }

        QCheckBox { color: #E3E3E3; font-size: 13px; spacing: 10px; }
        QCheckBox::indicator { width: 18px; height: 18px; }

        QPushButton {
            background-color: #4A90E2; color: white; border-radius: 6px;
            font-weight: bold; border: none; padding: 0 20px;
        }
        QPushButton:hover { background-color: #5BA3F5; }

        QPushButton#cancelBtn { background-color: #3C4043; }
        QPushButton#cancelBtn:hover { background-color: #4F5458; }
    )");
}