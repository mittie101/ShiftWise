#include "ReportsPage.h"
#include "app/AppSettings.h"
#include "domain/TimeUtils.h"
#include "domain/repositories/ScheduleRepository.h"
#include "domain/repositories/ShiftTemplateRepository.h"
#include "domain/repositories/EmployeeRepository.h"
#include "domain/repositories/RoleRepository.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QMessageBox>
#include <QHash>
#include <QBrush>
#include <QColor>
#include <QFont>
#include <algorithm>

ReportsPage::ReportsPage(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    // ── Fill-rate summary ─────────────────────────────────────────────────────
    m_summaryLabel = new QLabel;
    m_summaryLabel->setObjectName("sectionTitle");
    layout->addWidget(m_summaryLabel);

    // ── Employee hours table ──────────────────────────────────────────────────
    auto* hoursRow   = new QHBoxLayout;
    auto* hoursLabel = new QLabel("Employee Hours");
    hoursLabel->setObjectName("sectionTitle");
    hoursRow->addWidget(hoursLabel);
    hoursRow->addStretch();
    auto* exportBtn = new QPushButton("\xe2\x86\x93  Export CSV");  // ↓
    exportBtn->setToolTip("Export employee hours summary to CSV");
    hoursRow->addWidget(exportBtn);
    layout->addLayout(hoursRow);
    connect(exportBtn, &QPushButton::clicked, this, &ReportsPage::onExportCsvClicked);

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

    // ── Unfilled slots table ──────────────────────────────────────────────────
    auto* unfilledLabel = new QLabel("Unfilled Slots");
    unfilledLabel->setObjectName("sectionTitle");
    layout->addWidget(unfilledLabel);

    m_unfilledTable = new QTableWidget(0, 4, this);
    m_unfilledTable->setHorizontalHeaderLabels({"Day", "Shift", "Time", "Role"});
    m_unfilledTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_unfilledTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_unfilledTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    auto* uHdr = m_unfilledTable->horizontalHeader();
    uHdr->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    uHdr->setSectionResizeMode(1, QHeaderView::Stretch);
    uHdr->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    uHdr->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    layout->addWidget(m_unfilledTable);
}

void ReportsPage::loadWeek(const QString& weekStart)
{
    m_weekStart = weekStart;
    m_table->setRowCount(0);
    m_unfilledTable->setRowCount(0);

    ScheduleRepository      schedRepo;
    ShiftTemplateRepository shiftRepo;
    EmployeeRepository      empRepo;
    RoleRepository          roleRepo;

    const Schedule            schedule    = schedRepo.getOrCreate(weekStart);
    const QVector<Assignment> assignments = schedRepo.getAssignments(schedule.id);

    // Build lookup maps
    QHash<int, ShiftTemplate> shiftMap;
    for (const ShiftTemplate& st : shiftRepo.getAll())
        shiftMap.insert(st.id, st);

    QHash<int, QString> roleNames;
    for (const Role& r : roleRepo.getAll())
        roleNames.insert(r.id, r.name);

    // ── Fill-rate summary ─────────────────────────────────────────────────────
    int totalSlots = 0;
    for (const ShiftTemplate& st : shiftMap)
        totalSlots += st.staffRequired;
    const int filledSlots  = assignments.size();
    const int unfilledSlots = totalSlots - filledSlots;
    m_summaryLabel->setText(
        QString("%1 / %2 slots filled \xe2\x80\x94 %3 unfilled")  // "—"
            .arg(filledSlots).arg(totalSlots).arg(unfilledSlots));

    // ── Unfilled slots table ──────────────────────────────────────────────────
    QHash<QPair<int,int>, bool> filledKeys;
    for (const Assignment& a : assignments)
        filledKeys.insert(QPair<int,int>(a.shiftTemplateId, a.slotIndex), true);

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

    // Ensure ALL employees appear (even those with 0 scheduled hours)
    for (const Employee& e : empMap)
        scheduled.insert(e.id, scheduled.value(e.id, 0));

    // Sort by name
    QVector<int> empIds = scheduled.keys().toVector();
    std::sort(empIds.begin(), empIds.end(), [&empMap](int a, int b) {
        return empMap.value(a).name.compare(empMap.value(b).name, Qt::CaseInsensitive) < 0;
    });

    int totalSched = 0;
    int totalMax   = 0;
    int otCount    = 0;

    for (const int empId : empIds) {
        const int sched = scheduled.value(empId);

        const Employee& emp    = empMap[empId];
        const int       maxMin = (emp.id != -1) ? emp.maxWeeklyMinutes : threshold;
        const QString   name   = (emp.id != -1) ? emp.name
                                                 : QString("(employee %1)").arg(empId);

        const int row = m_table->rowCount();
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

        totalSched += sched;
        totalMax   += maxMin;
        if (overtime) ++otCount;
    }

    // ── Totals row ────────────────────────────────────────────────────────────
    if (!empIds.isEmpty()) {
        const int tRow = m_table->rowCount();
        m_table->insertRow(tRow);

        QFont bold;
        bold.setBold(true);

        auto makeItem = [&](const QString& text) {
            auto* item = new QTableWidgetItem(text);
            item->setFont(bold);
            item->setBackground(QBrush(QColor(40, 40, 90)));
            return item;
        };

        m_table->setItem(tRow, 0, makeItem("Totals"));
        m_table->setItem(tRow, 1, makeItem(TimeUtils::minutesToTimeString(totalSched, use24h)));
        m_table->setItem(tRow, 2, makeItem(TimeUtils::minutesToTimeString(totalMax, use24h)));
        m_table->setItem(tRow, 3, makeItem(
            otCount > 0 ? QString("%1 employee(s)").arg(otCount) : "None"));
        m_table->setItem(tRow, 4, makeItem(""));
    }

    // ── Unfilled slots ────────────────────────────────────────────────────────
    // Iterate shift templates sorted by day then start time
    QVector<ShiftTemplate> sortedShifts = shiftRepo.getAll();
    std::sort(sortedShifts.begin(), sortedShifts.end(),
        [](const ShiftTemplate& a, const ShiftTemplate& b) {
            if (a.dayOfWeek != b.dayOfWeek) return a.dayOfWeek < b.dayOfWeek;
            return a.startMinute < b.startMinute;
        });

    for (const ShiftTemplate& st : sortedShifts) {
        const QString day  = TimeUtils::dayOfWeekName(st.dayOfWeek);
        const QString time = TimeUtils::minutesToTimeString(st.startMinute, use24h)
            + QString::fromUtf8(" \xe2\x80\x93 ")
            + TimeUtils::minutesToTimeString(st.endMinute, use24h);
        const QString role = roleNames.value(st.roleId, QString("(id %1)").arg(st.roleId));

        for (int slot = 0; slot < st.staffRequired; ++slot) {
            if (filledKeys.contains(QPair<int,int>(st.id, slot))) continue;

            const int uRow = m_unfilledTable->rowCount();
            m_unfilledTable->insertRow(uRow);
            m_unfilledTable->setItem(uRow, 0, new QTableWidgetItem(day));
            m_unfilledTable->setItem(uRow, 1, new QTableWidgetItem(st.name));
            m_unfilledTable->setItem(uRow, 2, new QTableWidgetItem(time));
            m_unfilledTable->setItem(uRow, 3, new QTableWidgetItem(role));
        }
    }
}

void ReportsPage::onExportCsvClicked()
{
    const QString path = QFileDialog::getSaveFileName(
        this,
        "Export Hours Report as CSV",
        QString("hours_%1.csv").arg(m_weekStart),
        "CSV files (*.csv);;All files (*)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Export Failed",
            QString("Could not write to:\n%1").arg(path));
        return;
    }

    // Helper: quote a field if needed (RFC-4180)
    auto q = [](const QString& s) -> QString {
        if (s.contains(',') || s.contains('"') || s.contains('\n')) {
            QString esc = s;
            esc.replace('"', "\"\"");
            return '"' + esc + '"';
        }
        return s;
    };

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << "Employee,Scheduled (h:mm),Max (h:mm),Overtime,Fairness %\n";

    const int rows = m_table->rowCount();
    for (int r = 0; r < rows; ++r) {
        const QString name  = m_table->item(r, 0) ? m_table->item(r, 0)->text() : QString();
        if (name == "Totals") continue;   // skip the summary row
        const QString sched = m_table->item(r, 1) ? m_table->item(r, 1)->text() : QString();
        const QString max   = m_table->item(r, 2) ? m_table->item(r, 2)->text() : QString();
        const QString ot    = m_table->item(r, 3) ? m_table->item(r, 3)->text() : QString();
        const QString fair  = m_table->item(r, 4) ? m_table->item(r, 4)->text() : QString();
        out << q(name)  << ','
            << q(sched) << ','
            << q(max)   << ','
            << q(ot)    << ','
            << q(fair)  << '\n';
    }

    file.close();
}
