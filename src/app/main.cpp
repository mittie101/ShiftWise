#include "app/MainWindow.h"
#include "app/AppSettings.h"
#include "app/Logger.h"
#include "infra/DatabaseManager.h"
#include "infra/MigrationRunner.h"

#include <QApplication>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QMessageBox>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("ShiftWise");
    app.setApplicationVersion(SHIFTWISE_VERSION);
    app.setOrganizationName("ShiftWise");

    // ── Ensure app data directory exists ─────────────────────────────────────
    const QString dataDir = QStandardPaths::writableLocation(
        QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);

    // ── Logger ────────────────────────────────────────────────────────────────
    Logger::init(dataDir + "/shiftwise.log");
    Logger::info(QString("ShiftWise v%1 starting").arg(SHIFTWISE_VERSION));
    Logger::info(QString("Data directory: %1").arg(dataDir));

    // ── Stylesheet ────────────────────────────────────────────────────────────
    QFile qss(":/styles/dark.qss");
    if (qss.open(QIODevice::ReadOnly | QIODevice::Text)) {
        app.setStyleSheet(QString::fromUtf8(qss.readAll()));
        Logger::info("Dark stylesheet loaded.");
    } else {
        Logger::warning("Could not load dark.qss from resources.");
    }

    // ── AppSettings ───────────────────────────────────────────────────────────
    AppSettings& settings = AppSettings::instance();
    // Ensure DB path is set (uses default in AppDataLocation if not overridden)
    const QString dbPath = settings.databasePath();
    Logger::info(QString("Database path: %1").arg(dbPath));

    // ── Database + Migrations ─────────────────────────────────────────────────
    try {
        DatabaseManager& dm = DatabaseManager::instance();
        dm.setDatabasePath(dbPath);
        QSqlDatabase db = dm.openConnection(DatabaseManager::kMainConn);
        MigrationRunner::migrate(db);
        Logger::info("Database ready.");
    } catch (const std::exception& e) {
        Logger::error(QString("Database init failed: %1").arg(e.what()));
        QMessageBox::critical(
            nullptr,
            "ShiftWise — Database Error",
            QString("Failed to initialise the database:\n\n%1\n\nThe application cannot continue.")
                .arg(e.what())
        );
        return 1;
    }

    // ── Main Window ───────────────────────────────────────────────────────────
    MainWindow window;
    window.show();

    Logger::info("Main window shown. Entering event loop.");
    const int result = app.exec();

    // ── Shutdown ──────────────────────────────────────────────────────────────
    DatabaseManager::instance().closeConnection(DatabaseManager::kMainConn);
    Logger::info("ShiftWise exiting cleanly.");
    return result;
}
