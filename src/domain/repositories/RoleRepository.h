#pragma once
#include <optional>
#include <QVector>
#include <QString>
#include "domain/models/Role.h"

class RoleRepository {
public:
    explicit RoleRepository(const QString& connectionName = {});

    QVector<Role> getAll();
    std::optional<Role> getById(int id);
    int insert(Role role);
    bool update(Role role);
    bool remove(int id);

private:
    QString m_connectionName;
};
