#include "ReportsPage.h"
#include "app/AppSettings.h"
#include "domain/TimeUtils.h"
#include "domain/repositories/ScheduleRepository.h"
#include "domain/repositories/ShiftTemplateRepository.h"
#include "domain/repositories/EmployeeRepository.h"
#include <QVBoxLayout>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QHash>
#include <QBrush>
#include <QColor>

ReportsPage::ReportsPage(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);

    m_table = new QTableWidget(0, 5, this);
    m_table->setHorizontalHeaderLabels({
        "Employee", "Scheduled (h:mm)", "Max (h:mm)", "Overtime", "Fairness %"
    });
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);

    auto* hdr = m_table->horizontalHeader();
    hdr->setSectionResizeMode(0, QHeaderView::Stretch);
    hdr->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    hdr->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    hdr->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    hdr->setSectionResizeMode(4, QHeaderView::ResizeToContents);

    layout->addWidget(m_table);
}

void ReportsPage::loadWeek(const QString& weekStart)
{
    m_table->setRowCount(0);

    ScheduleRepository      schedRepo;
    ShiftTemplateRepository shiftRepo;
    EmployeeRepository      empRepo;

    const Schedule         schedule    = schedRepo.getOrCreate(weekStart);
    const QVector<Assignment> assignments = schedRepo.getAssignments(schedule.id);

    // Build lookup maps
    QHash<int, ShiftTemplate> shiftMap;
    for (const ShiftTemplate& st : shiftRepo.getAll())
        shiftMap.insert(st.id, st);

    QHash<int, Employee> empMap;
    for (const Employee& e : empRepo.getAll())
        empMap.insert(e.id, e);

    // Sum scheduled minutes per employee
    QHash<int, int> scheduled;   // employeeId -> minutes
    for (const Assignment& a : assignments) {
        const auto it = shiftMap.constFind(a.shiftTemplateId);
        if (it == shiftMap.constEnd()) continue;
        const ShiftTemplate& st = it.value();
        const int dur = (st.endMinute > st.startMinute)
            ? st.endMinute - st.startMinute
            : 1440 + st.endMinute - st.startMinute;
        scheduled[a.employeeId] += dur;
    }

    const int  threshold = AppSettings::instance().overtimeThresholdMinutes();
    const bool use24h    = AppSettings::instance().use24HourFormat();

    int row = 0;
    for (auto it = scheduled.constBegin(); it != scheduled.constEnd(); ++it, ++row) {
        const int empId = it.key();
        const int sched = it.value();

        const Employee& emp    = empMap[empId];
        const int       maxMin = (emp.id != -1) ? emp.maxWeeklyMinutes : threshold;
        const QString   name   = (emp.id != -1) ? emp.name
                                                 : QString("(employee %1)").arg(empId);

        m_table->insertRow(row);
        m_table->setItem(row, 0, new QTableWidgetItem(name));
        m_table->setItem(row, 1, new QTableWidgetItem(TimeUtils::minutesToTimeString(sched, use24h)));
        m_table->setItem(row, 2, new QTableWidgetItem(TimeUtils::minutesToTimeString(maxMin, use24h)));

        const bool overtime = (sched > threshold);
        auto* otItem = new QTableWidgetItem(overtime ? "Yes" : "No");
        if (overtime)
            otItem->setBackground(QBrush(QColor(180, 60, 60)));
        m_table->setItem(row, 3, otItem);

        const int fairness = (threshold > 0) ? (sched * 100 / threshold) : 0;
        m_table->setItem(row, 4, new QTableWidgetItem(QString("%1%").arg(fairness)));
    }
}
