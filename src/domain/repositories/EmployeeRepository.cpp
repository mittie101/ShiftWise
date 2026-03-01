#include "EmployeeRepository.h"
#include "infra/DatabaseManager.h"
#include "app/Logger.h"
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

static Employee empFromQuery(const QSqlQuery& q)
{
    Employee e;
    e.id               = q.value(0).toInt();
    e.name             = q.value(1).toString();
    e.maxWeeklyMinutes = q.value(2).toInt();
    e.priority         = q.value(3).toInt();
    return e;
}

static Role roleFromQuery(const QSqlQuery& q)
{
    Role r;
    r.id        = q.value(0).toInt();
    r.name      = q.value(1).toString();
    r.colourHex = q.value(2).toString();
    return r;
}

EmployeeRepository::EmployeeRepository(const QString& connectionName)
    : m_connectionName(connectionName.isEmpty()
          ? QString::fromLatin1(DatabaseManager::kMainConn)
          : connectionName)
{
}

QVector<Employee> EmployeeRepository::getAll()
{
    QVector<Employee> result;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    if (!q.exec(QStringLiteral(
            "SELECT id, name, maxWeeklyMinutes, priority FROM employees ORDER BY priority DESC, name"))) {
        Logger::error(QStringLiteral("EmployeeRepository::getAll: ") + q.lastError().text());
        return result;
    }
    while (q.next())
        result.append(empFromQuery(q));
    return result;
}

std::optional<Employee> EmployeeRepository::getById(int id)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "SELECT id, name, maxWeeklyMinutes, priority FROM employees WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        Logger::error(QStringLiteral("EmployeeRepository::getById: ") + q.lastError().text());
        return std::nullopt;
    }
    if (q.next())
        return empFromQuery(q);
    return std::nullopt;
}

int EmployeeRepository::insert(Employee employee)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "INSERT INTO employees (name, maxWeeklyMinutes, priority) "
        "VALUES (:name, :maxWeeklyMinutes, :priority)"));
    q.bindValue(QStringLiteral(":name"),             employee.name);
    q.bindValue(QStringLiteral(":maxWeeklyMinutes"),  employee.maxWeeklyMinutes);
    q.bindValue(QStringLiteral(":priority"),          employee.priority);
    if (!q.exec()) {
        Logger::error(QStringLiteral("EmployeeRepository::insert: ") + q.lastError().text());
        return -1;
    }
    return q.lastInsertId().toInt();
}

bool EmployeeRepository::update(Employee employee)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "UPDATE employees SET name = :name, maxWeeklyMinutes = :maxWeeklyMinutes, "
        "priority = :priority WHERE id = :id"));
    q.bindValue(QStringLiteral(":name"),             employee.name);
    q.bindValue(QStringLiteral(":maxWeeklyMinutes"),  employee.maxWeeklyMinutes);
    q.bindValue(QStringLiteral(":priority"),          employee.priority);
    q.bindValue(QStringLiteral(":id"),                employee.id);
    if (!q.exec()) {
        Logger::error(QStringLiteral("EmployeeRepository::update: ") + q.lastError().text());
        return false;
    }
    return q.numRowsAffected() > 0;
}

bool EmployeeRepository::remove(int id)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral("DELETE FROM employees WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        Logger::error(QStringLiteral("EmployeeRepository::remove: ") + q.lastError().text());
        return false;
    }
    return q.numRowsAffected() > 0;
}

QVector<Role> EmployeeRepository::getRolesForEmployee(int employeeId)
{
    QVector<Role> result;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "SELECT r.id, r.name, r.colourHex "
        "FROM roles r "
        "JOIN employee_roles er ON er.roleId = r.id "
        "WHERE er.employeeId = :employeeId "
        "ORDER BY r.name"));
    q.bindValue(QStringLiteral(":employeeId"), employeeId);
    if (!q.exec()) {
        Logger::error(QStringLiteral("EmployeeRepository::getRolesForEmployee: ") + q.lastError().text());
        return result;
    }
    while (q.next())
        result.append(roleFromQuery(q));
    return result;
}

bool EmployeeRepository::setRolesForEmployee(int employeeId, QVector<int> roleIds)
{
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    if (!db.transaction()) {
        Logger::error(QStringLiteral("EmployeeRepository::setRolesForEmployee: could not begin transaction"));
        return false;
    }

    QSqlQuery q(db);
    q.prepare(QStringLiteral("DELETE FROM employee_roles WHERE employeeId = :employeeId"));
    q.bindValue(QStringLiteral(":employeeId"), employeeId);
    if (!q.exec()) {
        Logger::error(QStringLiteral("EmployeeRepository::setRolesForEmployee (delete): ") + q.lastError().text());
        db.rollback();
        return false;
    }

    q.prepare(QStringLiteral(
        "INSERT INTO employee_roles (employeeId, roleId) VALUES (:employeeId, :roleId)"));
    for (int roleId : roleIds) {
        q.bindValue(QStringLiteral(":employeeId"), employeeId);
        q.bindValue(QStringLiteral(":roleId"),     roleId);
        if (!q.exec()) {
            Logger::error(QStringLiteral("EmployeeRepository::setRolesForEmployee (insert): ") + q.lastError().text());
            db.rollback();
            return false;
        }
    }

    if (!db.commit()) {
        Logger::error(QStringLiteral("EmployeeRepository::setRolesForEmployee: commit failed"));
        db.rollback();
        return false;
    }
    return true;
}
