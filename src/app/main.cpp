#include "app/MainWindow.h"
#include "app/AppSettings.h"
#include "app/Logger.h"
#include "infra/DatabaseManager.h"
#include "infra/MigrationRunner.h"

#include <QApplication>
#include <QIcon>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QMessageBox>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("ShiftWise");
    app.setApplicationVersion(SHIFTWISE_VERSION);
    app.setOrganizationName("Walking Fish Software");

    // ── Application icon (all sizes for crisp rendering at every scale) ───────
    QIcon appIcon;
    appIcon.addFile(":/icons/shiftwise_16.ico");
    appIcon.addFile(":/icons/shiftwise_32.ico");
    appIcon.addFile(":/icons/shiftwise_48.ico");
    appIcon.addFile(":/icons/shiftwise_256.ico");
    app.setWindowIcon(appIcon);

    // ── Ensure app data directory exists ─────────────────────────────────────
    const QString dataDir = QStandardPaths::writableLocation(
        QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);

    // ── Logger ────────────────────────────────────────────────────────────────
    Logger::init(dataDir + "/shiftwise.log");
    Logger::info(QString("ShiftWise v%1 starting").arg(SHIFTWISE_VERSION));
    Logger::info(QString("Data directory: %1").arg(dataDir));

    // ── AppSettings ───────────────────────────────────────────────────────────
    AppSettings& settings = AppSettings::instance();

    // ── Stylesheet ────────────────────────────────────────────────────────────
    const QString qssPath = settings.darkTheme()
        ? QStringLiteral(":/styles/dark.qss")
        : QStringLiteral(":/styles/light.qss");
    QFile qss(qssPath);
    if (qss.open(QIODevice::ReadOnly | QIODevice::Text)) {
        app.setStyleSheet(QString::fromUtf8(qss.readAll()));
        Logger::info(QString("%1 stylesheet loaded.").arg(settings.darkTheme() ? "Dark" : "Light"));
    } else {
        Logger::warning(QString("Could not load %1 from resources.").arg(qssPath));
    }

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
