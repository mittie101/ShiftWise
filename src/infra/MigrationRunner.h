#pragma once
#include <QtSql/QSqlDatabase>

// MigrationRunner — B11
// Runs pending migrations on startup in order.
// Idempotent: running twice is safe.
// Migrations are embedded in Qt resources (:/migrations/migration_NNN.sql).
class MigrationRunner {
public:
    // Runs all pending migrations on the given connection.
    // Throws std::runtime_error if a migration fails.
    static void migrate(QSqlDatabase& db);

private:
    static int  currentVersion(QSqlDatabase& db);
    static void applyMigration(QSqlDatabase& db, int version, const QString& resourcePath);
};
