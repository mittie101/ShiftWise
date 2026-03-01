#pragma once
#include <QObject>
#include <QString>

// AppSettings — singleton wrapping QSettings
// B7:  workWeekStartDay is display-only (0=Mon..6=Sun); DB weekStart is always Monday
// B14: Scheduler reads values through this singleton — never hardcoded
class AppSettings : public QObject {
    Q_OBJECT
public:
    static AppSettings& instance();

    // Database
    QString databasePath() const;
    void    setDatabasePath(const QString& path);

    // Last session
    QString lastWeekStart() const;        // ISO date, Monday
    void    setLastWeekStart(const QString& date);

    // Scheduling
    int  overtimeThresholdMinutes() const; // default 2400 = 40h
    void setOvertimeThresholdMinutes(int minutes);

    // Display
    int  workWeekStartDay() const;         // 0=Mon..6=Sun, default 0; display only (B7)
    void setWorkWeekStartDay(int day);

    bool use24HourFormat() const;
    void setUse24HourFormat(bool use24h);

    // Theme
    bool darkTheme() const;
    void setDarkTheme(bool dark);

signals:
    void settingsChanged();

private:
    AppSettings();
    ~AppSettings() = default;
    AppSettings(const AppSettings&) = delete;
    AppSettings& operator=(const AppSettings&) = delete;
};
