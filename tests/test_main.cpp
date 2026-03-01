#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>
#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>
#include <QCoreApplication>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>

#include "domain/repositories/RoleRepository.h"

// Ensure a QCoreApplication exists for the entire test session.
// Catch2WithMain provides main(); we hook in via a session listener.
namespace {

class QtAppListener : public Catch::EventListenerBase {
public:
    using Catch::EventListenerBase::EventListenerBase;

    void testRunStarting(Catch::TestRunInfo const&) override
    {
        // Create if not already present (only once)
        if (!QCoreApplication::instance()) {
            static int   argc = 0;
            static char* argv[] = { nullptr };
            m_app = std::make_unique<QCoreApplication>(argc, argv);
        }
    }

private:
    std::unique_ptr<QCoreApplication> m_app;
};

CATCH_REGISTER_LISTENER(QtAppListener)

} // namespace

// ── Phase 1 scaffold ─────────────────────────────────────────────────────────

TEST_CASE("Catch2 is wired correctly", "[scaffold]") {
    REQUIRE(1 + 1 == 2);
}

// ── Phase 4: RoleRepository smoke test ───────────────────────────────────────

namespace {

void setupRoleTable(const QString& connName)
{
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
    db.setDatabaseName(QStringLiteral(":memory:"));
    REQUIRE(db.open());

    QSqlQuery q(db);
    REQUIRE(q.exec(QStringLiteral(
        "CREATE TABLE roles ("
        "  id        INTEGER PRIMARY KEY,"
        "  name      TEXT    NOT NULL UNIQUE,"
        "  colourHex TEXT    NOT NULL DEFAULT '#e94560'"
        ")")));
}

void teardownConn(const QString& connName)
{
    QSqlDatabase::database(connName).close();
    QSqlDatabase::removeDatabase(connName);
}

} // namespace

TEST_CASE("RoleRepository: insert, getById, update, remove", "[domain][role]")
{
    const QString kConn = QStringLiteral("test_role_conn");
    setupRoleTable(kConn);

    RoleRepository repo(kConn);

    // Insert
    Role r;
    r.name      = QStringLiteral("Barista");
    r.colourHex = QStringLiteral("#aabbcc");

    int newId = repo.insert(r);
    REQUIRE(newId > 0);

    // getById — fields match
    auto fetched = repo.getById(newId);
    REQUIRE(fetched.has_value());
    CHECK(fetched->id        == newId);
    CHECK(fetched->name      == QStringLiteral("Barista"));
    CHECK(fetched->colourHex == QStringLiteral("#aabbcc"));

    // update
    fetched->name      = QStringLiteral("Manager");
    fetched->colourHex = QStringLiteral("#112233");
    REQUIRE(repo.update(*fetched));

    auto updated = repo.getById(newId);
    REQUIRE(updated.has_value());
    CHECK(updated->name      == QStringLiteral("Manager"));
    CHECK(updated->colourHex == QStringLiteral("#112233"));

    // remove
    REQUIRE(repo.remove(newId));
    CHECK_FALSE(repo.getById(newId).has_value());

    // remove non-existent returns false
    CHECK_FALSE(repo.remove(newId));

    teardownConn(kConn);
}
