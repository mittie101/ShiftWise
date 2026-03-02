#include "Logger.h"
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QRecursiveMutex>
#include <QMutexLocker>

static QString         g_logPath;
static QRecursiveMutex g_mutex;

void Logger::init(const QString& logFilePath) {
    QMutexLocker lock(&g_mutex);
    g_logPath = logFilePath;
    write("INFO", "=== ShiftWise started ===");
}

void Logger::info(const QString& message) {
    write("INFO", message);
}

void Logger::warning(const QString& message) {
    write("WARN", message);
}

void Logger::error(const QString& message) {
    write("ERROR", message);
}

void Logger::write(const QString& level, const QString& message) {
    QMutexLocker lock(&g_mutex);
    if (g_logPath.isEmpty()) return;

    QFile file(g_logPath);
    if (!file.open(QIODevice::Append | QIODevice::Text)) return;

    QTextStream out(&file);
    out << QDateTime::currentDateTime().toString(Qt::ISODate)
        << " [" << level << "] "
        << message << "\n";
}
