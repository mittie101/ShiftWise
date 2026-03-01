#pragma once
#include <QString>

struct ShiftTemplate {
    int id = -1;
    QString name;
    int dayOfWeek = 0;       // 0=Mon..6=Sun
    int startMinute = 0;
    int endMinute = 0;       // endMinute <= startMinute means midnight crossing
    int roleId = -1;
    int staffRequired = 1;
};
