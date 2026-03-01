#pragma once
#include <QWidget>

class QLineEdit;
class QSpinBox;
class QCheckBox;
class QLabel;

class SettingsPage : public QWidget {
    Q_OBJECT
public:
    explicit SettingsPage(QWidget* parent = nullptr);

private slots:
    void onBrowseClicked();
    void onSaveClicked();

private:
    QLineEdit* m_dbPathEdit     = nullptr;
    QSpinBox*  m_overtimeSpin   = nullptr;
    QCheckBox* m_use24hCheck    = nullptr;
    QCheckBox* m_darkThemeCheck = nullptr;
    QLabel*    m_statusLabel    = nullptr;
};
