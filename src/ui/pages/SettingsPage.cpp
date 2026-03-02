#include "SettingsPage.h"
#include "app/AppSettings.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QApplication>
#include <QFile>

SettingsPage::SettingsPage(QWidget* parent)
    : QWidget(parent)
{
    auto* outer = new QVBoxLayout(this);
    outer->setAlignment(Qt::AlignTop);
    outer->setContentsMargins(24, 24, 24, 24);
    outer->setSpacing(16);

    auto* form = new QFormLayout;
    form->setSpacing(10);

    // DB path row
    auto* dbRow    = new QHBoxLayout;
    m_dbPathEdit   = new QLineEdit;
    auto* browseBtn = new QPushButton("Browse\xe2\x80\xa6");  // "Browse…"
    browseBtn->setMaximumWidth(90);
    dbRow->addWidget(m_dbPathEdit);
    dbRow->addWidget(browseBtn);
    form->addRow("Database path:", dbRow);

    // Overtime threshold (stored in minutes; show in hours)
    m_overtimeSpin = new QSpinBox;
    m_overtimeSpin->setRange(1, 168);
    m_overtimeSpin->setSuffix(" h");
    form->addRow("Overtime threshold:", m_overtimeSpin);

    // 24-hour time format
    m_use24hCheck = new QCheckBox("Use 24-hour time format");
    form->addRow("Time format:", m_use24hCheck);

    // Dark theme
    m_darkThemeCheck = new QCheckBox("Enable dark theme");
    form->addRow("Theme:", m_darkThemeCheck);

    // Work week start day (display-only: affects calendar column order)
    m_weekStartCombo = new QComboBox;
    const char* kDayNames[] = {"Monday","Tuesday","Wednesday","Thursday","Friday","Saturday","Sunday"};
    for (int i = 0; i < 7; ++i)
        m_weekStartCombo->addItem(QString::fromLatin1(kDayNames[i]), i);
    form->addRow("Week starts on:", m_weekStartCombo);

    outer->addLayout(form);

    auto* saveBtn = new QPushButton("Save");
    saveBtn->setObjectName("primaryBtn");
    saveBtn->setMaximumWidth(100);
    outer->addWidget(saveBtn);

    m_statusLabel = new QLabel;
    outer->addWidget(m_statusLabel);

    // Read current values from AppSettings
    const AppSettings& s = AppSettings::instance();
    m_dbPathEdit->setText(s.databasePath());
    m_overtimeSpin->setValue(s.overtimeThresholdMinutes() / 60);
    m_use24hCheck->setChecked(s.use24HourFormat());
    m_darkThemeCheck->setChecked(s.darkTheme());
    m_weekStartCombo->setCurrentIndex(s.workWeekStartDay());

    connect(browseBtn, &QPushButton::clicked, this, &SettingsPage::onBrowseClicked);
    connect(saveBtn,   &QPushButton::clicked, this, &SettingsPage::onSaveClicked);
}

void SettingsPage::onBrowseClicked()
{
    const QString path = QFileDialog::getSaveFileName(
        this,
        "Select Database File",
        m_dbPathEdit->text(),
        "SQLite Database (*.db);;All Files (*)");
    if (!path.isEmpty())
        m_dbPathEdit->setText(path);
}

void SettingsPage::onSaveClicked()
{
    AppSettings& s = AppSettings::instance();
    const QString oldDbPath = s.databasePath();
    const QString newDbPath = m_dbPathEdit->text().trimmed();

    s.setDatabasePath(newDbPath);
    s.setOvertimeThresholdMinutes(m_overtimeSpin->value() * 60);
    s.setUse24HourFormat(m_use24hCheck->isChecked());
    s.setDarkTheme(m_darkThemeCheck->isChecked());
    s.setWorkWeekStartDay(m_weekStartCombo->currentData().toInt());

    // Apply theme immediately
    const QString qssPath = m_darkThemeCheck->isChecked()
        ? QStringLiteral(":/styles/dark.qss")
        : QStringLiteral(":/styles/light.qss");
    QFile qss(qssPath);
    if (qss.open(QIODevice::ReadOnly | QIODevice::Text))
        qApp->setStyleSheet(QString::fromUtf8(qss.readAll()));

    if (newDbPath != oldDbPath)
        m_statusLabel->setText(
            "Database path changed \xe2\x80\x94 restart the application to take effect.");
    else
        m_statusLabel->setText("Settings saved.");
}
