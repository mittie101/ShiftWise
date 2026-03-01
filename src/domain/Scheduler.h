#pragma once
#include <QString>

// Scheduler — greedy best-fit algorithm for a single week.
//
// For each unlocked slot (shiftTemplateId, slotIndex):
//   1. Find employees with the required role, availability covering the shift,
//      and remaining weekly capacity.
//   2. Sort candidates: priority DESC, then scheduledMinutes ASC (fairness).
//   3. Assign the best candidate; skip the slot if no one qualifies.
//
// Locked assignments are never touched.
class Scheduler {
public:
    // Returns the number of slots filled.
    static int generateWeek(const QString& weekStart,
                             const QString& connectionName = {});
};
