#include "MigrationRunner.h"
#include "app/Logger.h"
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QFile>
#include <QString>
#include <stdexcept>

// ── Migration registry ────────────────────────────────────────────────────────
// Add new entries here when adding future migrations.
static const struct {
    int     version;
    QString resourcePath;
} kMigrations[] = {
    { 1, ":/migrations/migration_001.sql" },
    // { 2, ":/migrations/migration_002.sql" },
};

static constexpr int kMigrationCount = sizeof(kMigrations) / sizeof(kMigrations[0]);

// ─────────────────────────────────────────────────────────────────────────────

int MigrationRunner::currentVersion(QSqlDatabase& db) {
    QSqlQuery q(db);
    // schema_version may not exist yet on first run
    q.exec("CREATE TABLE IF NOT EXISTS schema_version (version INTEGER PRIMARY KEY);");
    if (!q.exec("SELECT MAX(version) FROM schema_version;")) return 0;
    if (!q.next()) return 0;
    return q.value(0).isNull() ? 0 : q.value(0).toInt();
}

void MigrationRunner::applyMigration(QSqlDatabase& db, int version, const QString& resourcePath) {
    QFile f(resourcePath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        throw std::runtime_error(
            ("MigrationRunner: cannot open resource: " + resourcePath).toStdString());
    }

    const QString sql = QString::fromUtf8(f.readAll());
    f.close();

    // Split on ';' — works for well-formed SQL migrations.
    // Strips comments and blank statements.
    QSqlQuery q(db);
    const QStringList statements = sql.split(';', Qt::SkipEmptyParts);
    for (const QString& raw : statements) {
        const QString stmt = raw.trimmed();
        if (stmt.isEmpty()) continue;
        if (stmt.startsWith("--"))  continue;

        if (!q.exec(stmt)) {
            throw std::runtime_error(
                ("Migration " + QString::number(version) + " failed on statement ["
                 + stmt.left(60) + "...]: "
                 + q.lastError().text()).toStdString());
        }
    }

    // Record version applied
    QSqlQuery vq(db);
    vq.prepare("INSERT OR IGNORE INTO schema_version(version) VALUES (:v);");
    vq.bindValue(":v", version);
    vq.exec();

    Logger::info(QString("Migration %1 applied successfully.").arg(version));
}

void MigrationRunner::migrate(QSqlDatabase& db) {
    const int current = currentVersion(db);
    Logger::info(QString("MigrationRunner: current schema version = %1").arg(current));

    int applied = 0;
    for (int i = 0; i < kMigrationCount; ++i) {
        if (kMigrations[i].version <= current) continue;
        applyMigration(db, kMigrations[i].version, kMigrations[i].resourcePath);
        ++applied;
    }

    if (applied == 0) {
        Logger::info("MigrationRunner: schema up to date, no migrations needed.");
    } else {
        Logger::info(QString("MigrationRunner: %1 migration(s) applied.").arg(applied));
    }
}
