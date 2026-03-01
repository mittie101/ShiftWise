#include "RoleRepository.h"
#include "infra/DatabaseManager.h"
#include "app/Logger.h"
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

static Role fromQuery(const QSqlQuery& q)
{
    Role r;
    r.id        = q.value(0).toInt();
    r.name      = q.value(1).toString();
    r.colourHex = q.value(2).toString();
    return r;
}

RoleRepository::RoleRepository(const QString& connectionName)
    : m_connectionName(connectionName.isEmpty()
          ? QString::fromLatin1(DatabaseManager::kMainConn)
          : connectionName)
{
}

QVector<Role> RoleRepository::getAll()
{
    QVector<Role> result;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    if (!q.exec(QStringLiteral("SELECT id, name, colourHex FROM roles ORDER BY name"))) {
        Logger::error(QStringLiteral("RoleRepository::getAll: ") + q.lastError().text());
        return result;
    }
    while (q.next())
        result.append(fromQuery(q));
    return result;
}

std::optional<Role> RoleRepository::getById(int id)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral("SELECT id, name, colourHex FROM roles WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        Logger::error(QStringLiteral("RoleRepository::getById: ") + q.lastError().text());
        return std::nullopt;
    }
    if (q.next())
        return fromQuery(q);
    return std::nullopt;
}

int RoleRepository::insert(Role role)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "INSERT INTO roles (name, colourHex) VALUES (:name, :colourHex)"));
    q.bindValue(QStringLiteral(":name"),      role.name);
    q.bindValue(QStringLiteral(":colourHex"), role.colourHex);
    if (!q.exec()) {
        Logger::error(QStringLiteral("RoleRepository::insert: ") + q.lastError().text());
        return -1;
    }
    return q.lastInsertId().toInt();
}

bool RoleRepository::update(Role role)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "UPDATE roles SET name = :name, colourHex = :colourHex WHERE id = :id"));
    q.bindValue(QStringLiteral(":name"),      role.name);
    q.bindValue(QStringLiteral(":colourHex"), role.colourHex);
    q.bindValue(QStringLiteral(":id"),        role.id);
    if (!q.exec()) {
        Logger::error(QStringLiteral("RoleRepository::update: ") + q.lastError().text());
        return false;
    }
    return q.numRowsAffected() > 0;
}

bool RoleRepository::remove(int id)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral("DELETE FROM roles WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        Logger::error(QStringLiteral("RoleRepository::remove: ") + q.lastError().text());
        return false;
    }
    return q.numRowsAffected() > 0;
}
