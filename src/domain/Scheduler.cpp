#include "Scheduler.h"
#include "domain/repositories/ScheduleRepository.h"
#include "domain/repositories/ShiftTemplateRepository.h"
#include "domain/repositories/EmployeeRepository.h"
#include "domain/repositories/AvailabilityRepository.h"
#include "domain/models/Employee.h"
#include "domain/models/ShiftTemplate.h"
#include "domain/models/Availability.h"
#include "domain/models/Assignment.h"
#include <QHash>
#include <QMap>
#include <QPair>
#include <QSet>
#include <QVector>
#include <algorithm>

int Scheduler::generateWeek(const QString& weekStart, const QString& connectionName)
{
    ScheduleRepository      schedRepo(connectionName);
    ShiftTemplateRepository shiftRepo(connectionName);
    EmployeeRepository      empRepo(connectionName);
    AvailabilityRepository  availRepo(connectionName);

    // ── Load data ─────────────────────────────────────────────────────────────
    const Schedule            schedule   = schedRepo.getOrCreate(weekStart);
    const QVector<Assignment> existing   = schedRepo.getAssignments(schedule.id);
    const QVector<Employee>   employees  = empRepo.getAll();
    const QVector<ShiftTemplate> shifts  = shiftRepo.getAll(); // sorted dayOfWeek, startMinute

    // Shift template lookup
    QHash<int, ShiftTemplate> shiftMap;
    for (const ShiftTemplate& st : shifts)
        shiftMap.insert(st.id, st);

    // Existing assignment lookup: (shiftTemplateId, slotIndex) -> Assignment
    QMap<QPair<int,int>, Assignment> assignMap;
    for (const Assignment& a : existing)
        assignMap.insert(QPair<int,int>(a.shiftTemplateId, a.slotIndex), a);

    // Employee roles: empId -> set of roleIds
    QHash<int, QSet<int>> empRoles;
    for (const Employee& e : employees) {
        QSet<int> roleSet;
        for (const Role& r : empRepo.getRolesForEmployee(e.id))
            roleSet.insert(r.id);
        empRoles.insert(e.id, roleSet);
    }

    // Employee availability: empId -> list of Availability records
    QHash<int, QVector<Availability>> empAvail;
    for (const Employee& e : employees)
        empAvail.insert(e.id, availRepo.getForEmployee(e.id));

    // ── Seed scheduled minutes from locked assignments ─────────────────────────
    QHash<int, int> scheduledMinutes; // empId -> minutes scheduled this week
    for (const Assignment& a : existing) {
        if (!a.isLocked) continue;
        const auto it = shiftMap.constFind(a.shiftTemplateId);
        if (it == shiftMap.constEnd()) continue;
        const ShiftTemplate& st = it.value();
        const int dur = (st.endMinute > st.startMinute)
            ? st.endMinute - st.startMinute
            : 1440 + st.endMinute - st.startMinute;
        scheduledMinutes[a.employeeId] += dur;
    }

    // Track employees already assigned per shift (to avoid double-booking within
    // the same shift template, including locked slots)
    QHash<int, QSet<int>> assignedPerShift; // shiftTemplateId -> set of empIds
    for (const Assignment& a : existing) {
        if (a.isLocked)
            assignedPerShift[a.shiftTemplateId].insert(a.employeeId);
    }

    // ── Generate ──────────────────────────────────────────────────────────────
    int filled = 0;

    for (const ShiftTemplate& st : shifts) {
        const int dur = (st.endMinute > st.startMinute)
            ? st.endMinute - st.startMinute
            : 1440 + st.endMinute - st.startMinute;

        for (int slot = 0; slot < st.staffRequired; ++slot) {
            const QPair<int,int> key(st.id, slot);

            // Skip locked assignments
            const auto existIt = assignMap.constFind(key);
            if (existIt != assignMap.constEnd() && existIt->isLocked)
                continue;

            // Find eligible employees
            QVector<const Employee*> eligible;
            for (const Employee& e : employees) {
                // Must have the required role
                if (!empRoles.value(e.id).contains(st.roleId))
                    continue;

                // Must not already be in another slot of this same shift
                if (assignedPerShift.value(st.id).contains(e.id))
                    continue;

                // Must have availability covering the shift on that day
                bool available = false;
                for (const Availability& av : empAvail.value(e.id)) {
                    if (av.dayOfWeek != st.dayOfWeek)
                        continue;
                    if (st.endMinute > st.startMinute) {
                        // Normal (non-midnight-crossing) shift
                        if (av.startMinute <= st.startMinute && av.endMinute >= st.endMinute) {
                            available = true;
                            break;
                        }
                    } else {
                        // Midnight-crossing shift: require availability start covers shift start
                        if (av.startMinute <= st.startMinute) {
                            available = true;
                            break;
                        }
                    }
                }
                if (!available) continue;

                // Must have remaining weekly capacity
                if (scheduledMinutes.value(e.id) + dur > e.maxWeeklyMinutes)
                    continue;

                eligible.append(&e);
            }

            if (eligible.isEmpty())
                continue;

            // Sort: priority DESC, then already-scheduled minutes ASC (fairness)
            std::stable_sort(eligible.begin(), eligible.end(),
                [&scheduledMinutes](const Employee* a, const Employee* b) {
                    if (a->priority != b->priority)
                        return a->priority > b->priority;
                    return scheduledMinutes.value(a->id) < scheduledMinutes.value(b->id);
                });

            const Employee* best = eligible.first();

            // Build and upsert the assignment
            Assignment a;
            if (existIt != assignMap.constEnd()) {
                a            = existIt.value();
                a.employeeId = best->id;
                a.isLocked   = false;
            } else {
                a.scheduleId      = schedule.id;
                a.shiftTemplateId = st.id;
                a.slotIndex       = slot;
                a.employeeId      = best->id;
                a.isLocked        = false;
            }
            schedRepo.upsertAssignment(a);

            // Update running state
            scheduledMinutes[best->id] += dur;
            assignedPerShift[st.id].insert(best->id);
            ++filled;
        }
    }

    return filled;
}
