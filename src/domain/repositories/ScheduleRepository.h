#pragma once
#include <optional>
#include <QVector>
#include <QString>
#include "domain/models/Schedule.h"
#include "domain/models/Assignment.h"

class ScheduleRepository {
public:
    explicit ScheduleRepository(const QString& connectionName = {});

    QVector<Schedule> getAll();
    std::optional<Schedule> getById(int id);
    int insert(Schedule schedule);
    bool update(Schedule schedule);
    bool remove(int id);

    Schedule getOrCreate(const QString& weekStart);
    QVector<Assignment> getAssignments(int scheduleId);
    bool upsertAssignment(Assignment assignment);
    bool clearUnlocked(int scheduleId);

private:
    QString m_connectionName;
};
