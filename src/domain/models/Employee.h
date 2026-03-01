#pragma once
#include <QString>

struct Employee {
    int id = -1;
    QString name;
    int maxWeeklyMinutes = 2400;
    int priority = 0;
};
