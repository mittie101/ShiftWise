#pragma once

struct Assignment {
    int id = -1;
    int scheduleId = -1;
    int shiftTemplateId = -1;
    int slotIndex = 0;
    int employeeId = -1;
    bool isLocked = false;   // stored as INTEGER 0/1 in DB
};
