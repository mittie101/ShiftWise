#pragma once
#include <QString>

struct Schedule {
    int id = -1;
    QString weekStart;   // ISO "YYYY-MM-DD", always Monday
    QString createdAt;
};
