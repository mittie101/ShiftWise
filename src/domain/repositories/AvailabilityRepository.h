#pragma once
#include <optional>
#include <QVector>
#include <QString>
#include "domain/models/Availability.h"

class AvailabilityRepository {
public:
    explicit AvailabilityRepository(const QString& connectionName = {});

    QVector<Availability> getAll();
    std::optional<Availability> getById(int id);
    int insert(Availability availability);
    bool update(Availability availability);
    bool remove(int id);

    QVector<Availability> getForEmployee(int employeeId);
    bool replaceForEmployee(int employeeId, QVector<Availability> availabilities);

private:
    QString m_connectionName;
};
