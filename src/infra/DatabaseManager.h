#pragma once
#include <QtSql/QSqlDatabase>
#include <QString>

// DatabaseManager — singleton
// B4:  One QSqlDatabase connection per thread. openConnection(name) is
//      the ONLY permitted way to open a connection in this app.
// B10: PRAGMA foreign_keys = ON is enforced on every connection, immediately
//      after open. Never bypass via raw QSqlDatabase.
class DatabaseManager {
public:
    static DatabaseManager& instance();

    void setDatabasePath(const QString& path);
    QString databasePath() const;

    // Open (or return existing open) named connection.
    // Applies PRAGMA foreign_keys = ON before returning.
    // Throws std::runtime_error on failure.
    QSqlDatabase openConnection(const QString& connectionName);

    // Close and remove a named connection.
    void closeConnection(const QString& connectionName);

    // Convenience: the main thread connection name.
    static constexpr const char* kMainConn      = "main_conn";
    static constexpr const char* kSchedulerConn = "scheduler_conn";

private:
    DatabaseManager() = default;
    ~DatabaseManager() = default;
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    QString m_dbPath;
};
