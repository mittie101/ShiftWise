#include "AvailabilityRepository.h"
#include "infra/DatabaseManager.h"
#include "app/Logger.h"
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

static Availability fromQuery(const QSqlQuery& q)
{
    Availability a;
    a.id          = q.value(0).toInt();
    a.employeeId  = q.value(1).toInt();
    a.dayOfWeek   = q.value(2).toInt();
    a.startMinute = q.value(3).toInt();
    a.endMinute   = q.value(4).toInt();
    return a;
}

AvailabilityRepository::AvailabilityRepository(const QString& connectionName)
    : m_connectionName(connectionName.isEmpty()
          ? QString::fromLatin1(DatabaseManager::kMainConn)
          : connectionName)
{
}

QVector<Availability> AvailabilityRepository::getAll()
{
    QVector<Availability> result;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    if (!q.exec(QStringLiteral(
            "SELECT id, employeeId, dayOfWeek, startMinute, endMinute FROM availability"))) {
        Logger::error(QStringLiteral("AvailabilityRepository::getAll: ") + q.lastError().text());
        return result;
    }
    while (q.next())
        result.append(fromQuery(q));
    return result;
}

std::optional<Availability> AvailabilityRepository::getById(int id)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "SELECT id, employeeId, dayOfWeek, startMinute, endMinute "
        "FROM availability WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        Logger::error(QStringLiteral("AvailabilityRepository::getById: ") + q.lastError().text());
        return std::nullopt;
    }
    if (q.next())
        return fromQuery(q);
    return std::nullopt;
}

int AvailabilityRepository::insert(Availability availability)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "INSERT INTO availability (employeeId, dayOfWeek, startMinute, endMinute) "
        "VALUES (:employeeId, :dayOfWeek, :startMinute, :endMinute)"));
    q.bindValue(QStringLiteral(":employeeId"),  availability.employeeId);
    q.bindValue(QStringLiteral(":dayOfWeek"),   availability.dayOfWeek);
    q.bindValue(QStringLiteral(":startMinute"), availability.startMinute);
    q.bindValue(QStringLiteral(":endMinute"),   availability.endMinute);
    if (!q.exec()) {
        Logger::error(QStringLiteral("AvailabilityRepository::insert: ") + q.lastError().text());
        return -1;
    }
    return q.lastInsertId().toInt();
}

bool AvailabilityRepository::update(Availability availability)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "UPDATE availability SET employeeId = :employeeId, dayOfWeek = :dayOfWeek, "
        "startMinute = :startMinute, endMinute = :endMinute WHERE id = :id"));
    q.bindValue(QStringLiteral(":employeeId"),  availability.employeeId);
    q.bindValue(QStringLiteral(":dayOfWeek"),   availability.dayOfWeek);
    q.bindValue(QStringLiteral(":startMinute"), availability.startMinute);
    q.bindValue(QStringLiteral(":endMinute"),   availability.endMinute);
    q.bindValue(QStringLiteral(":id"),          availability.id);
    if (!q.exec()) {
        Logger::error(QStringLiteral("AvailabilityRepository::update: ") + q.lastError().text());
        return false;
    }
    return q.numRowsAffected() > 0;
}

bool AvailabilityRepository::remove(int id)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral("DELETE FROM availability WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        Logger::error(QStringLiteral("AvailabilityRepository::remove: ") + q.lastError().text());
        return false;
    }
    return q.numRowsAffected() > 0;
}

QVector<Availability> AvailabilityRepository::getForEmployee(int employeeId)
{
    QVector<Availability> result;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "SELECT id, employeeId, dayOfWeek, startMinute, endMinute "
        "FROM availability WHERE employeeId = :id ORDER BY dayOfWeek, startMinute"));
    q.bindValue(QStringLiteral(":id"), employeeId);
    if (!q.exec()) {
        Logger::error(QStringLiteral("AvailabilityRepository::getForEmployee: ") + q.lastError().text());
        return result;
    }
    while (q.next())
        result.append(fromQuery(q));
    return result;
}

bool AvailabilityRepository::replaceForEmployee(int employeeId, QVector<Availability> availabilities)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    if (!db.transaction()) {
        Logger::error(QStringLiteral("AvailabilityRepository::replaceForEmployee: could not begin transaction"));
        return false;
    }

    QSqlQuery q(db);
    q.prepare(QStringLiteral("DELETE FROM availability WHERE employeeId = :employeeId"));
    q.bindValue(QStringLiteral(":employeeId"), employeeId);
    if (!q.exec()) {
        Logger::error(QStringLiteral("AvailabilityRepository::replaceForEmployee (delete): ") + q.lastError().text());
        db.rollback();
        return false;
    }

    q.prepare(QStringLiteral(
        "INSERT INTO availability (employeeId, dayOfWeek, startMinute, endMinute) "
        "VALUES (:employeeId, :dayOfWeek, :startMinute, :endMinute)"));
    for (const Availability& a : availabilities) {
        q.bindValue(QStringLiteral(":employeeId"),  employeeId);
        q.bindValue(QStringLiteral(":dayOfWeek"),   a.dayOfWeek);
        q.bindValue(QStringLiteral(":startMinute"), a.startMinute);
        q.bindValue(QStringLiteral(":endMinute"),   a.endMinute);
        if (!q.exec()) {
            Logger::error(QStringLiteral("AvailabilityRepository::replaceForEmployee (insert): ") + q.lastError().text());
            db.rollback();
            return false;
        }
    }

    if (!db.commit()) {
        Logger::error(QStringLiteral("AvailabilityRepository::replaceForEmployee: commit failed"));
        db.rollback();
        return false;
    }
    return true;
}
