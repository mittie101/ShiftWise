#pragma once
#include <optional>
#include <QVector>
#include <QString>
#include "domain/models/ShiftTemplate.h"

class ShiftTemplateRepository {
public:
    explicit ShiftTemplateRepository(const QString& connectionName = {});

    QVector<ShiftTemplate> getAll();
    std::optional<ShiftTemplate> getById(int id);
    int insert(ShiftTemplate shiftTemplate);
    bool update(ShiftTemplate shiftTemplate);
    bool remove(int id);

private:
    QString m_connectionName;
};
