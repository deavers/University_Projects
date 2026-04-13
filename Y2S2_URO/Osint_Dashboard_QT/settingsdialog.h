#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QFrame>

// ==============================================================================
// SETTINGS DIALOG MODULE (Second Sub-window)
// ==============================================================================
class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);

private:
    void setupUI();
    void applyStyles();
};

#endif // SETTINGSDIALOG_H