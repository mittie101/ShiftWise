#include "SettingsPage.h"
#include "app/AppSettings.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>

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
    s.setDatabasePath(m_dbPathEdit->text().trimmed());
    s.setOvertimeThresholdMinutes(m_overtimeSpin->value() * 60);
    s.setUse24HourFormat(m_use24hCheck->isChecked());
    s.setDarkTheme(m_darkThemeCheck->isChecked());
    m_statusLabel->setText("Settings saved.");
}
