#include "DatabaseManager.h"
#include "app/Logger.h"
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>
#include <stdexcept>

DatabaseManager& DatabaseManager::instance() {
    static DatabaseManager inst;
    return inst;
}

void DatabaseManager::setDatabasePath(const QString& path) {
    m_dbPath = path;
}

QString DatabaseManager::databasePath() const {
    return m_dbPath;
}

QSqlDatabase DatabaseManager::openConnection(const QString& connectionName) {
    if (m_dbPath.isEmpty()) {
        throw std::runtime_error("DatabaseManager: database path not set before openConnection()");
    }

    QSqlDatabase db;

    if (QSqlDatabase::contains(connectionName)) {
        db = QSqlDatabase::database(connectionName);
        if (!db.isOpen()) {
            if (!db.open()) {
                throw std::runtime_error(
                    ("DB reopen failed [" + connectionName + "]: "
                     + db.lastError().text()).toStdString());
            }
        }
    } else {
        db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName(m_dbPath);
        if (!db.open()) {
            QSqlDatabase::removeDatabase(connectionName);
            throw std::runtime_error(
                ("DB open failed [" + connectionName + "]: "
                 + db.lastError().text()).toStdString());
        }
    }

    // B10: Always enforce foreign keys — on every connection, every open
    QSqlQuery pragma(db);
    if (!pragma.exec("PRAGMA foreign_keys = ON;")) {
        throw std::runtime_error(
            ("PRAGMA foreign_keys failed [" + connectionName + "]: "
             + pragma.lastError().text()).toStdString());
    }

    // WAL mode: better concurrent read performance
    pragma.exec("PRAGMA journal_mode = WAL;");

    Logger::info(QString("DB connection opened: %1").arg(connectionName));
    return db;
}

void DatabaseManager::closeConnection(const QString& connectionName) {
    if (QSqlDatabase::contains(connectionName)) {
        QSqlDatabase::database(connectionName).close();
        QSqlDatabase::removeDatabase(connectionName);
        Logger::info(QString("DB connection closed: %1").arg(connectionName));
    }
}
