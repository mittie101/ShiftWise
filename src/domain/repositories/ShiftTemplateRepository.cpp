#include "ShiftTemplateRepository.h"
#include "infra/DatabaseManager.h"
#include "app/Logger.h"
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

static ShiftTemplate fromQuery(const QSqlQuery& q)
{
    ShiftTemplate st;
    st.id           = q.value(0).toInt();
    st.name         = q.value(1).toString();
    st.dayOfWeek    = q.value(2).toInt();
    st.startMinute  = q.value(3).toInt();
    st.endMinute    = q.value(4).toInt();
    st.roleId       = q.value(5).toInt();
    st.staffRequired = q.value(6).toInt();
    return st;
}

ShiftTemplateRepository::ShiftTemplateRepository(const QString& connectionName)
    : m_connectionName(connectionName.isEmpty()
          ? QString::fromLatin1(DatabaseManager::kMainConn)
          : connectionName)
{
}

QVector<ShiftTemplate> ShiftTemplateRepository::getAll()
{
    QVector<ShiftTemplate> result;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    if (!q.exec(QStringLiteral(
            "SELECT id, name, dayOfWeek, startMinute, endMinute, roleId, staffRequired "
            "FROM shift_templates ORDER BY dayOfWeek, startMinute"))) {
        Logger::error(QStringLiteral("ShiftTemplateRepository::getAll: ") + q.lastError().text());
        return result;
    }
    while (q.next())
        result.append(fromQuery(q));
    return result;
}

std::optional<ShiftTemplate> ShiftTemplateRepository::getById(int id)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "SELECT id, name, dayOfWeek, startMinute, endMinute, roleId, staffRequired "
        "FROM shift_templates WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        Logger::error(QStringLiteral("ShiftTemplateRepository::getById: ") + q.lastError().text());
        return std::nullopt;
    }
    if (q.next())
        return fromQuery(q);
    return std::nullopt;
}

int ShiftTemplateRepository::insert(ShiftTemplate shiftTemplate)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "INSERT INTO shift_templates (name, dayOfWeek, startMinute, endMinute, roleId, staffRequired) "
        "VALUES (:name, :dayOfWeek, :startMinute, :endMinute, :roleId, :staffRequired)"));
    q.bindValue(QStringLiteral(":name"),          shiftTemplate.name);
    q.bindValue(QStringLiteral(":dayOfWeek"),     shiftTemplate.dayOfWeek);
    q.bindValue(QStringLiteral(":startMinute"),   shiftTemplate.startMinute);
    q.bindValue(QStringLiteral(":endMinute"),     shiftTemplate.endMinute);
    q.bindValue(QStringLiteral(":roleId"),        shiftTemplate.roleId);
    q.bindValue(QStringLiteral(":staffRequired"), shiftTemplate.staffRequired);
    if (!q.exec()) {
        Logger::error(QStringLiteral("ShiftTemplateRepository::insert: ") + q.lastError().text());
        return -1;
    }
    return q.lastInsertId().toInt();
}

bool ShiftTemplateRepository::update(ShiftTemplate shiftTemplate)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "UPDATE shift_templates SET name = :name, dayOfWeek = :dayOfWeek, "
        "startMinute = :startMinute, endMinute = :endMinute, "
        "roleId = :roleId, staffRequired = :staffRequired WHERE id = :id"));
    q.bindValue(QStringLiteral(":name"),          shiftTemplate.name);
    q.bindValue(QStringLiteral(":dayOfWeek"),     shiftTemplate.dayOfWeek);
    q.bindValue(QStringLiteral(":startMinute"),   shiftTemplate.startMinute);
    q.bindValue(QStringLiteral(":endMinute"),     shiftTemplate.endMinute);
    q.bindValue(QStringLiteral(":roleId"),        shiftTemplate.roleId);
    q.bindValue(QStringLiteral(":staffRequired"), shiftTemplate.staffRequired);
    q.bindValue(QStringLiteral(":id"),            shiftTemplate.id);
    if (!q.exec()) {
        Logger::error(QStringLiteral("ShiftTemplateRepository::update: ") + q.lastError().text());
        return false;
    }
    return q.numRowsAffected() > 0;
}

bool ShiftTemplateRepository::remove(int id)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral("DELETE FROM shift_templates WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        Logger::error(QStringLiteral("ShiftTemplateRepository::remove: ") + q.lastError().text());
        return false;
    }
    return q.numRowsAffected() > 0;
}
