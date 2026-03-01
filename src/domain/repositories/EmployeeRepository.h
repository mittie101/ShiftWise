#pragma once
#include <optional>
#include <QVector>
#include <QString>
#include "domain/models/Employee.h"
#include "domain/models/Role.h"

class EmployeeRepository {
public:
    explicit EmployeeRepository(const QString& connectionName = {});

    QVector<Employee> getAll();
    std::optional<Employee> getById(int id);
    int insert(Employee employee);
    bool update(Employee employee);
    bool remove(int id);

    QVector<Role> getRolesForEmployee(int employeeId);
    bool setRolesForEmployee(int employeeId, QVector<int> roleIds);

private:
    QString m_connectionName;
};
