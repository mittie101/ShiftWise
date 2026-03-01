#include "AppSettings.h"
#include <QSettings>
#include <QStandardPaths>
#include <QDir>

AppSettings& AppSettings::instance() {
    static AppSettings inst;
    return inst;
}

AppSettings::AppSettings() {}

static QSettings& s() {
    static QSettings settings("ShiftWise", "ShiftWise");
    return settings;
}

// ── Database ────────────────────────────────────────────────────────────────

QString AppSettings::databasePath() const {
    const QString defaultPath = QStandardPaths::writableLocation(
        QStandardPaths::AppDataLocation) + "/shiftwise.db";
    return s().value("db/path", defaultPath).toString();
}

void AppSettings::setDatabasePath(const QString& path) {
    s().setValue("db/path", path);
    emit settingsChanged();
}

// ── Last session ─────────────────────────────────────────────────────────────

QString AppSettings::lastWeekStart() const {
    return s().value("session/lastWeekStart", "").toString();
}

void AppSettings::setLastWeekStart(const QString& date) {
    s().setValue("session/lastWeekStart", date);
}

// ── Scheduling ───────────────────────────────────────────────────────────────

int AppSettings::overtimeThresholdMinutes() const {
    return s().value("scheduling/overtimeThresholdMinutes", 2400).toInt();
}

void AppSettings::setOvertimeThresholdMinutes(int minutes) {
    s().setValue("scheduling/overtimeThresholdMinutes", minutes);
    emit settingsChanged();
}

// ── Display ──────────────────────────────────────────────────────────────────

int AppSettings::workWeekStartDay() const {
    // 0=Mon..6=Sun; display-only per B7 — never affects DB queries
    return s().value("display/workWeekStartDay", 0).toInt();
}

void AppSettings::setWorkWeekStartDay(int day) {
    s().setValue("display/workWeekStartDay", day);
    emit settingsChanged();
}

bool AppSettings::use24HourFormat() const {
    return s().value("display/use24HourFormat", true).toBool();
}

void AppSettings::setUse24HourFormat(bool use24h) {
    s().setValue("display/use24HourFormat", use24h);
    emit settingsChanged();
}

// ── Theme ─────────────────────────────────────────────────────────────────────

bool AppSettings::darkTheme() const {
    return s().value("display/darkTheme", true).toBool();
}

void AppSettings::setDarkTheme(bool dark) {
    s().setValue("display/darkTheme", dark);
    emit settingsChanged();
}
