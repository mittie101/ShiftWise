#include "ScheduleRepository.h"
#include "infra/DatabaseManager.h"
#include "app/Logger.h"
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QDateTime>

static Schedule scheduleFromQuery(const QSqlQuery& q)
{
    Schedule s;
    s.id        = q.value(0).toInt();
    s.weekStart = q.value(1).toString();
    s.createdAt = q.value(2).toString();
    return s;
}

static Assignment assignmentFromQuery(const QSqlQuery& q)
{
    Assignment a;
    a.id             = q.value(0).toInt();
    a.scheduleId     = q.value(1).toInt();
    a.shiftTemplateId = q.value(2).toInt();
    a.slotIndex      = q.value(3).toInt();
    a.employeeId     = q.value(4).toInt();
    a.isLocked       = q.value(5).toInt() != 0;
    return a;
}

ScheduleRepository::ScheduleRepository(const QString& connectionName)
    : m_connectionName(connectionName.isEmpty()
          ? QString::fromLatin1(DatabaseManager::kMainConn)
          : connectionName)
{
}

QVector<Schedule> ScheduleRepository::getAll()
{
    QVector<Schedule> result;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    if (!q.exec(QStringLiteral("SELECT id, weekStart, createdAt FROM schedules ORDER BY weekStart"))) {
        Logger::error(QStringLiteral("ScheduleRepository::getAll: ") + q.lastError().text());
        return result;
    }
    while (q.next())
        result.append(scheduleFromQuery(q));
    return result;
}

std::optional<Schedule> ScheduleRepository::getById(int id)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral("SELECT id, weekStart, createdAt FROM schedules WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        Logger::error(QStringLiteral("ScheduleRepository::getById: ") + q.lastError().text());
        return std::nullopt;
    }
    if (q.next())
        return scheduleFromQuery(q);
    return std::nullopt;
}

int ScheduleRepository::insert(Schedule schedule)
{
    if (schedule.createdAt.isEmpty())
        schedule.createdAt = QDateTime::currentDateTime().toString(Qt::ISODate);

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "INSERT INTO schedules (weekStart, createdAt) VALUES (:weekStart, :createdAt)"));
    q.bindValue(QStringLiteral(":weekStart"),  schedule.weekStart);
    q.bindValue(QStringLiteral(":createdAt"),  schedule.createdAt);
    if (!q.exec()) {
        Logger::error(QStringLiteral("ScheduleRepository::insert: ") + q.lastError().text());
        return -1;
    }
    return q.lastInsertId().toInt();
}

bool ScheduleRepository::update(Schedule schedule)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "UPDATE schedules SET weekStart = :weekStart, createdAt = :createdAt WHERE id = :id"));
    q.bindValue(QStringLiteral(":weekStart"), schedule.weekStart);
    q.bindValue(QStringLiteral(":createdAt"), schedule.createdAt);
    q.bindValue(QStringLiteral(":id"),        schedule.id);
    if (!q.exec()) {
        Logger::error(QStringLiteral("ScheduleRepository::update: ") + q.lastError().text());
        return false;
    }
    return q.numRowsAffected() > 0;
}

bool ScheduleRepository::remove(int id)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral("DELETE FROM schedules WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        Logger::error(QStringLiteral("ScheduleRepository::remove: ") + q.lastError().text());
        return false;
    }
    return q.numRowsAffected() > 0;
}

Schedule ScheduleRepository::getOrCreate(const QString& weekStart)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);

    // INSERT OR IGNORE never modifies existing rows — no transaction needed
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "INSERT OR IGNORE INTO schedules (weekStart, createdAt) VALUES (:weekStart, :createdAt)"));
    q.bindValue(QStringLiteral(":weekStart"), weekStart);
    q.bindValue(QStringLiteral(":createdAt"), QDateTime::currentDateTime().toString(Qt::ISODate));
    if (!q.exec())
        Logger::warning(QStringLiteral("ScheduleRepository::getOrCreate (insert): ") + q.lastError().text());

    q.prepare(QStringLiteral("SELECT id, weekStart, createdAt FROM schedules WHERE weekStart = :weekStart"));
    q.bindValue(QStringLiteral(":weekStart"), weekStart);
    if (!q.exec()) {
        Logger::error(QStringLiteral("ScheduleRepository::getOrCreate (select): ") + q.lastError().text());
        return {};
    }
    if (q.next())
        return scheduleFromQuery(q);
    return {};
}

QVector<Assignment> ScheduleRepository::getAssignments(int scheduleId)
{
    QVector<Assignment> result;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "SELECT id, scheduleId, shiftTemplateId, slotIndex, employeeId, isLocked "
        "FROM assignments WHERE scheduleId = :id ORDER BY shiftTemplateId, slotIndex"));
    q.bindValue(QStringLiteral(":id"), scheduleId);
    if (!q.exec()) {
        Logger::error(QStringLiteral("ScheduleRepository::getAssignments: ") + q.lastError().text());
        return result;
    }
    while (q.next())
        result.append(assignmentFromQuery(q));
    return result;
}

bool ScheduleRepository::upsertAssignment(Assignment assignment)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "INSERT INTO assignments (scheduleId, shiftTemplateId, slotIndex, employeeId, isLocked) "
        "VALUES (:scheduleId, :shiftTemplateId, :slotIndex, :employeeId, :isLocked) "
        "ON CONFLICT(scheduleId, shiftTemplateId, slotIndex) "
        "DO UPDATE SET employeeId = excluded.employeeId, isLocked = excluded.isLocked"));
    q.bindValue(QStringLiteral(":scheduleId"),      assignment.scheduleId);
    q.bindValue(QStringLiteral(":shiftTemplateId"), assignment.shiftTemplateId);
    q.bindValue(QStringLiteral(":slotIndex"),       assignment.slotIndex);
    q.bindValue(QStringLiteral(":employeeId"),      assignment.employeeId);
    q.bindValue(QStringLiteral(":isLocked"),        assignment.isLocked ? 1 : 0);
    if (!q.exec()) {
        Logger::error(QStringLiteral("ScheduleRepository::upsertAssignment: ") + q.lastError().text());
        return false;
    }
    return true;
}

bool ScheduleRepository::removeAssignment(int assignmentId)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral("DELETE FROM assignments WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), assignmentId);
    if (!q.exec()) {
        Logger::error(QStringLiteral("ScheduleRepository::removeAssignment: ") + q.lastError().text());
        return false;
    }
    return true;
}

bool ScheduleRepository::clearUnlocked(int scheduleId)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "DELETE FROM assignments WHERE scheduleId = :id AND isLocked = 0"));
    q.bindValue(QStringLiteral(":id"), scheduleId);
    if (!q.exec()) {
        Logger::error(QStringLiteral("ScheduleRepository::clearUnlocked: ") + q.lastError().text());
        return false;
    }
    return true;  // true even if 0 rows deleted
}
