#pragma once
#include <QString>

// Logger — writes to %APPDATA%/ShiftWise/shiftwise.log
// Thread-safe via QMutex.
// Usage: Logger::info("message");  Logger::error("message");
class Logger {
public:
    static void init(const QString& logFilePath);
    static void info(const QString& message);
    static void warning(const QString& message);
    static void error(const QString& message);

private:
    static void write(const QString& level, const QString& message);
};
