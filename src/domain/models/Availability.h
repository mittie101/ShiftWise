#pragma once

struct Availability {
    int id = -1;
    int employeeId = -1;
    int dayOfWeek = 0;   // 0=Mon..6=Sun
    int startMinute = 0;
    int endMinute = 0;
};
