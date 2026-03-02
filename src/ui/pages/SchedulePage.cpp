#include "SchedulePage.h"
#include "app/AppSettings.h"
#include "domain/TimeUtils.h"
#include "domain/repositories/ScheduleRepository.h"
#include "domain/repositories/ShiftTemplateRepository.h"
#include "domain/repositories/EmployeeRepository.h"
#include "domain/repositories/AvailabilityRepository.h"
#include "domain/repositories/RoleRepository.h"
#include "domain/models/Availability.h"
#include "domain/models/Role.h"
#include <QDate>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QPushButton>
#include <QCheckBox>
#include <QDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QHash>
#include <QFont>
#include <QBrush>
#include <QColor>
#include <algorithm>

// Column indices
static constexpr int kColDay      = 0;
static constexpr int kColShift    = 1;
static constexpr int kColTime     = 2;
static constexpr int kColRole     = 3;
static constexpr int kColSlot     = 4;
static constexpr int kColAssigned = 5;
static constexpr int kColLocked   = 6;

SchedulePage::SchedulePage(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);

    m_tabs = new QTabWidget;

    // ── List tab ──────────────────────────────────────────────────────────────
    m_table = new QTableWidget(0, 7);
    m_table->setHorizontalHeaderLabels({
        "Day", "Shift", "Time", "Role", "Slot", "Assigned Employee", "Locked"
    });
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);

    auto* hdr = m_table->horizontalHeader();
    hdr->setSectionResizeMode(kColDay,      QHeaderView::ResizeToContents);
    hdr->setSectionResizeMode(kColShift,    QHeaderView::Stretch);
    hdr->setSectionResizeMode(kColTime,     QHeaderView::ResizeToContents);
    hdr->setSectionResizeMode(kColRole,     QHeaderView::ResizeToContents);
    hdr->setSectionResizeMode(kColSlot,     QHeaderView::ResizeToContents);
    hdr->setSectionResizeMode(kColAssigned, QHeaderView::ResizeToContents);
    hdr->setSectionResizeMode(kColLocked,   QHeaderView::ResizeToContents);

    m_tabs->addTab(m_table, "List");

    // ── Calendar tab ──────────────────────────────────────────────────────────
    m_calTable = new QTableWidget(0, 7);
    m_calTable->setHorizontalHeaderLabels(
        {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"});
    m_calTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_calTable->setSelectionMode(QAbstractItemView::NoSelection);
    auto* calHdr = m_calTable->horizontalHeader();
    for (int c = 0; c < 7; ++c)
        calHdr->setSectionResizeMode(c, QHeaderView::Stretch);
    m_calTable->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    m_tabs->addTab(m_calTable, "Calendar");

    layout->addWidget(m_tabs);
}

void SchedulePage::loadWeek(const QString& weekStart)
{
    m_weekStart = weekStart;
    m_table->setRowCount(0);
    m_rows.clear();
    m_assignments.clear();
    m_shiftMap.clear();

    ScheduleRepository      schedRepo;
    ShiftTemplateRepository shiftRepo;
    EmployeeRepository      empRepo;
    RoleRepository          roleRepo;

    const Schedule schedule = schedRepo.getOrCreate(weekStart);

    // Build assignment map: (shiftTemplateId, slotIndex) -> Assignment
    for (const Assignment& a : schedRepo.getAssignments(schedule.id))
        m_assignments.insert(QPair<int,int>(a.shiftTemplateId, a.slotIndex), a);

    // Build lookup maps
    QHash<int, Employee> empMap;
    for (const Employee& e : empRepo.getAll())
        empMap.insert(e.id, e);

    QHash<int, Role> roleMap;
    for (const Role& r : roleRepo.getAll())
        roleMap.insert(r.id, r);

    const bool use24h    = AppSettings::instance().use24HourFormat();
    const int  startDay  = AppSettings::instance().workWeekStartDay();

    // Cache all shift templates (used by list, calendar and conflict detection)
    QVector<ShiftTemplate> allShifts = shiftRepo.getAll();
    for (const ShiftTemplate& st : allShifts)
        m_shiftMap.insert(st.id, st);

    // Sort by work-week-relative day order, then by start time
    std::sort(allShifts.begin(), allShifts.end(),
        [startDay](const ShiftTemplate& a, const ShiftTemplate& b) {
            const int da = (a.dayOfWeek - startDay + 7) % 7;
            const int db = (b.dayOfWeek - startDay + 7) % 7;
            if (da != db) return da < db;
            return a.startMinute < b.startMinute;
        });

    const QDate weekStartDate = QDate::fromString(weekStart, Qt::ISODate);
    int lastDay = -1;

    for (const ShiftTemplate& st : allShifts) {
        // ── Day section header ─────────────────────────────────────────────────
        if (st.dayOfWeek != lastDay) {
            lastDay = st.dayOfWeek;
            const int hdrRow = m_table->rowCount();
            m_table->insertRow(hdrRow);

            RowMeta hdr;
            hdr.isHeader = true;
            m_rows.append(hdr);

            // e.g. "Monday  2 Jun 2025"
            const QDate dayDate = weekStartDate.addDays(st.dayOfWeek);
            const QString dayLabel = TimeUtils::dayOfWeekName(st.dayOfWeek)
                + "    " + dayDate.toString("d MMM yyyy");
            auto* hdrItem = new QTableWidgetItem(dayLabel);
            QFont f = hdrItem->font();
            f.setBold(true);
            hdrItem->setFont(f);
            hdrItem->setBackground(QBrush(QColor(40, 40, 90)));
            hdrItem->setFlags(Qt::ItemIsEnabled);   // not selectable
            m_table->setItem(hdrRow, 0, hdrItem);
            m_table->setSpan(hdrRow, 0, 1, 7);
        }

        const QString dayStr  = TimeUtils::dayOfWeekName(st.dayOfWeek);
        const QString timeStr = TimeUtils::minutesToTimeString(st.startMinute, use24h)
            + QString::fromUtf8(" \xe2\x80\x93 ")       // " – "
            + TimeUtils::minutesToTimeString(st.endMinute, use24h);
        const Role&   role    = roleMap.value(st.roleId);
        const QString roleStr = role.id != -1 ? role.name
                                              : QString("(id %1)").arg(st.roleId);

        for (int slot = 0; slot < st.staffRequired; ++slot) {
            const int row = m_table->rowCount();
            m_table->insertRow(row);

            RowMeta meta;
            meta.shiftTemplateId = st.id;
            meta.slotIndex       = slot;
            meta.scheduleId      = schedule.id;
            m_rows.append(meta);

            m_table->setItem(row, kColDay,   new QTableWidgetItem(dayStr));
            m_table->setItem(row, kColShift, new QTableWidgetItem(st.name));
            m_table->setItem(row, kColTime,  new QTableWidgetItem(timeStr));

            auto* roleItem = new QTableWidgetItem(roleStr);
            if (role.id != -1) {
                const QColor bg(role.colourHex);
                if (bg.isValid()) {
                    roleItem->setBackground(QBrush(bg));
                    const int brightness = bg.red() * 299 + bg.green() * 587 + bg.blue() * 114;
                    roleItem->setForeground(QBrush(QColor(brightness > 128000 ? "#000000" : "#ffffff")));
                }
            }
            m_table->setItem(row, kColRole, roleItem);
            m_table->setItem(row, kColSlot,  new QTableWidgetItem(QString::number(slot + 1)));

            // Assigned employee button
            const auto aIt = m_assignments.constFind(QPair<int,int>(st.id, slot));
            const bool hasAssign = (aIt != m_assignments.constEnd());

            QString btnLabel;
            if (hasAssign) {
                const Employee& emp = empMap[aIt->employeeId];
                btnLabel = (emp.id != -1) ? emp.name
                                          : QString("(emp %1)").arg(aIt->employeeId);
            } else {
                btnLabel = QString::fromUtf8("\xe2\x80\x94 assign \xe2\x80\x94"); // "— assign —"
            }

            auto* btn = new QPushButton(btnLabel);
            m_table->setCellWidget(row, kColAssigned, btn);
            connect(btn, &QPushButton::clicked, this, [this, row]() {
                onAssignClicked(row);
            });

            // Locked checkbox (centred in a container widget)
            auto* cb          = new QCheckBox;
            auto* cbContainer = new QWidget;
            auto* cbLayout    = new QHBoxLayout(cbContainer);
            cbLayout->addWidget(cb);
            cbLayout->setAlignment(Qt::AlignCenter);
            cbLayout->setContentsMargins(0, 0, 0, 0);
            cb->setChecked(hasAssign && aIt->isLocked);
            m_table->setCellWidget(row, kColLocked, cbContainer);

            connect(cb, &QCheckBox::toggled, this, [this, row](bool v) {
                onLockToggled(row, v);
            });
        }
    }

    refreshCalendar();
}

void SchedulePage::onAssignClicked(int row)
{
    if (row < 0 || row >= m_rows.size()) return;
    if (m_rows[row].isHeader) return;
    const RowMeta meta = m_rows[row];

    // Build inline assignment dialog
    EmployeeRepository     empRepo;
    AvailabilityRepository availRepo;
    const QVector<Employee> employees = empRepo.getAll();
    const ShiftTemplate& curSt = m_shiftMap.value(meta.shiftTemplateId);

    // ── Compute scheduled minutes per employee (excluding this slot) ──────────
    QHash<int, int> scheduledMins;
    for (auto it = m_assignments.constBegin(); it != m_assignments.constEnd(); ++it) {
        if (it.key() == QPair<int,int>(meta.shiftTemplateId, meta.slotIndex)) continue;
        const ShiftTemplate& st = m_shiftMap.value(it.key().first);
        if (st.id == -1) continue;
        const int dur = (st.endMinute > st.startMinute)
            ? st.endMinute - st.startMinute
            : 1440 + st.endMinute - st.startMinute;
        scheduledMins[it->employeeId] += dur;
    }
    const int curDur = (curSt.endMinute > curSt.startMinute)
        ? curSt.endMinute - curSt.startMinute
        : 1440 + curSt.endMinute - curSt.startMinute;

    // ── Categorise each employee ──────────────────────────────────────────────
    struct EmpEntry { Employee emp; bool eligible = true; QString reason; };
    QVector<EmpEntry> entries;

    for (const Employee& e : employees) {
        EmpEntry en; en.emp = e;

        if (curSt.id != -1) {
            // Role check
            bool hasRole = false;
            for (const Role& r : empRepo.getRolesForEmployee(e.id))
                if (r.id == curSt.roleId) { hasRole = true; break; }

            if (!hasRole) {
                en.eligible = false; en.reason = "wrong role";
            } else {
                // Availability check
                bool avail = false;
                for (const Availability& av : availRepo.getForEmployee(e.id)) {
                    if (av.dayOfWeek != curSt.dayOfWeek) continue;
                    if (curSt.endMinute > curSt.startMinute) {
                        if (av.startMinute <= curSt.startMinute && av.endMinute >= curSt.endMinute)
                            { avail = true; break; }
                    } else {
                        if (av.startMinute <= curSt.startMinute) { avail = true; break; }
                    }
                }
                if (!avail) {
                    en.eligible = false; en.reason = "unavailable";
                } else if (scheduledMins.value(e.id) + curDur > e.maxWeeklyMinutes) {
                    en.eligible = false; en.reason = "over capacity";
                } else {
                    // Overlap check
                    for (auto it = m_assignments.constBegin(); it != m_assignments.constEnd(); ++it) {
                        if (it->employeeId != e.id) continue;
                        if (it.key() == QPair<int,int>(meta.shiftTemplateId, meta.slotIndex)) continue;
                        const ShiftTemplate& other = m_shiftMap.value(it.key().first);
                        if (other.id == -1 || other.dayOfWeek != curSt.dayOfWeek) continue;
                        const int s1 = curSt.startMinute, e1 = (curSt.endMinute > s1) ? curSt.endMinute : 1440;
                        const int s2 = other.startMinute, e2 = (other.endMinute > s2) ? other.endMinute : 1440;
                        if (s1 < e2 && s2 < e1) { en.eligible = false; en.reason = "shift conflict"; break; }
                    }
                }
            }
        }
        entries.append(en);
    }

    // ── Build combo ───────────────────────────────────────────────────────────
    QDialog dlg(this);
    dlg.setWindowTitle("Assign Employee");
    auto* layout = new QVBoxLayout(&dlg);

    auto* combo = new QComboBox;
    combo->addItem(QString::fromUtf8("\xe2\x80\x94 unassign \xe2\x80\x94"), -1);

    for (const EmpEntry& en : entries)
        if (en.eligible) combo->addItem(en.emp.name, en.emp.id);

    const bool hasIneligible = std::any_of(entries.begin(), entries.end(),
        [](const EmpEntry& en) { return !en.eligible; });
    if (hasIneligible) {
        const int sepIdx = combo->count();
        combo->addItem(QString::fromUtf8("\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80 Ineligible \xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"), -2);
        if (auto* model = qobject_cast<QStandardItemModel*>(combo->model())) {
            auto* sep = model->item(sepIdx);
            if (sep) { sep->setEnabled(false); sep->setForeground(QColor(120, 120, 120)); }
        }
        for (const EmpEntry& en : entries) {
            if (en.eligible) continue;
            const int idx = combo->count();
            combo->addItem(QString("%1  (%2)").arg(en.emp.name, en.reason), en.emp.id);
            combo->setItemData(idx, QColor(140, 140, 140), Qt::ForegroundRole);
        }
    }

    // Pre-select current assignment
    const auto aIt = m_assignments.constFind(QPair<int,int>(meta.shiftTemplateId, meta.slotIndex));
    if (aIt != m_assignments.constEnd()) {
        for (int i = 1; i < combo->count(); ++i) {
            if (combo->itemData(i).toInt() == aIt->employeeId) {
                combo->setCurrentIndex(i);
                break;
            }
        }
    }

    layout->addWidget(new QLabel("Select employee:"));
    layout->addWidget(combo);
    auto* btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(btns);
    connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) return;

    const int selectedEmpId = combo->currentData().toInt();
    ScheduleRepository schedRepo;

    // ── Conflict check ────────────────────────────────────────────────────────
    if (selectedEmpId != -1) {
        const ShiftTemplate& curSt = m_shiftMap.value(meta.shiftTemplateId);
        if (curSt.id != -1) {
            for (auto it = m_assignments.constBegin(); it != m_assignments.constEnd(); ++it) {
                if (it->employeeId != selectedEmpId) continue;
                if (it.key() == QPair<int,int>(meta.shiftTemplateId, meta.slotIndex)) continue;
                const ShiftTemplate& other = m_shiftMap.value(it.key().first);
                if (other.id == -1 || other.dayOfWeek != curSt.dayOfWeek) continue;
                const int s1 = curSt.startMinute;
                const int e1 = (curSt.endMinute > s1) ? curSt.endMinute : 1440;
                const int s2 = other.startMinute;
                const int e2 = (other.endMinute > s2) ? other.endMinute : 1440;
                if (s1 < e2 && s2 < e1) {
                    const auto reply = QMessageBox::warning(this, "Scheduling Conflict",
                        QString("This employee is already assigned to \"%1\" which "
                                "overlaps on the same day.\n\nAssign anyway?")
                            .arg(other.name),
                        QMessageBox::Yes | QMessageBox::No);
                    if (reply != QMessageBox::Yes) return;
                    break;
                }
            }
        }
    }

    if (selectedEmpId == -1) {
        // Unassign: delete the existing assignment if any
        auto it = m_assignments.find(QPair<int,int>(meta.shiftTemplateId, meta.slotIndex));
        if (it != m_assignments.end()) {
            schedRepo.removeAssignment(it->id);
            m_assignments.erase(it);
        }
    } else {
        // Assign or reassign
        Assignment a;
        auto it = m_assignments.find(QPair<int,int>(meta.shiftTemplateId, meta.slotIndex));
        if (it != m_assignments.end()) {
            a = it.value();
            a.employeeId = selectedEmpId;
        } else {
            a.scheduleId      = meta.scheduleId;
            a.shiftTemplateId = meta.shiftTemplateId;
            a.slotIndex       = meta.slotIndex;
            a.employeeId      = selectedEmpId;
            a.isLocked        = false;
        }
        schedRepo.upsertAssignment(a);

        // Reload assignments to get the up-to-date id for any newly inserted row
        m_assignments.clear();
        for (const Assignment& loaded : schedRepo.getAssignments(meta.scheduleId))
            m_assignments.insert(QPair<int,int>(loaded.shiftTemplateId, loaded.slotIndex), loaded);
    }

    // Update button label
    if (auto* btn = qobject_cast<QPushButton*>(m_table->cellWidget(row, kColAssigned))) {
        if (selectedEmpId == -1) {
            btn->setText(QString::fromUtf8("\xe2\x80\x94 assign \xe2\x80\x94"));
        } else {
            auto opt = empRepo.getById(selectedEmpId);
            btn->setText(opt ? opt->name : QString("(emp %1)").arg(selectedEmpId));
        }
    }

    // Reset locked checkbox (new assignment starts unlocked)
    if (QWidget* container = m_table->cellWidget(row, kColLocked)) {
        if (auto* cb = container->findChild<QCheckBox*>()) {
            cb->blockSignals(true);
            cb->setChecked(false);
            cb->blockSignals(false);
        }
    }

    emit saved();
    refreshCalendar();
}

void SchedulePage::onLockToggled(int row, bool locked)
{
    if (row < 0 || row >= m_rows.size()) return;
    if (m_rows[row].isHeader) return;
    const RowMeta meta = m_rows[row];

    auto it = m_assignments.find(QPair<int,int>(meta.shiftTemplateId, meta.slotIndex));
    if (it == m_assignments.end()) {
        // No assignment — silently revert the checkbox
        if (QWidget* container = m_table->cellWidget(row, kColLocked)) {
            if (auto* cb = container->findChild<QCheckBox*>()) {
                cb->blockSignals(true);
                cb->setChecked(false);
                cb->blockSignals(false);
            }
        }
        return;
    }

    it->isLocked = locked;
    ScheduleRepository schedRepo;
    schedRepo.upsertAssignment(it.value());
    emit saved();
}

void SchedulePage::refreshCalendar()
{
    EmployeeRepository empRepo;
    QVector<Employee> employees = empRepo.getAll();
    std::sort(employees.begin(), employees.end(), [](const Employee& a, const Employee& b) {
        return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
    });

    const int   startDay      = AppSettings::instance().workWeekStartDay(); // 0=Mon..6=Sun
    const QDate weekStartDate = QDate::fromString(m_weekStart, Qt::ISODate);

    // Update column headers: "Mon\n2 Jun"
    QStringList colHdrs;
    for (int i = 0; i < 7; ++i) {
        const int  dow     = (startDay + i) % 7;
        const QDate colDate = weekStartDate.addDays(dow);  // weekStart is always Monday (dow=0)
        colHdrs << TimeUtils::dayOfWeekShort(dow) + "\n" + colDate.toString("d MMM");
    }
    m_calTable->setHorizontalHeaderLabels(colHdrs);

    m_calTable->clearContents();
    m_calTable->setRowCount(employees.size());

    QStringList rowHdrs;
    QHash<int, int> empRowIdx;
    for (int i = 0; i < employees.size(); ++i) {
        rowHdrs << employees[i].name;
        empRowIdx.insert(employees[i].id, i);
    }
    m_calTable->setVerticalHeaderLabels(rowHdrs);

    // Initialise every cell to empty
    for (int r = 0; r < m_calTable->rowCount(); ++r)
        for (int c = 0; c < 7; ++c)
            m_calTable->setItem(r, c, new QTableWidgetItem());

    const bool use24h = AppSettings::instance().use24HourFormat();

    // Fill cells from current assignment map
    for (auto it = m_assignments.constBegin(); it != m_assignments.constEnd(); ++it) {
        const Assignment& a = it.value();
        const ShiftTemplate& st = m_shiftMap.value(a.shiftTemplateId);
        if (st.id == -1) continue;

        const int empRow = empRowIdx.value(a.employeeId, -1);
        if (empRow < 0) continue;

        // Map dayOfWeek to column based on work week start day
        const int col = (st.dayOfWeek - startDay + 7) % 7;
        auto* item = m_calTable->item(empRow, col);
        if (!item) continue;

        const QString entry = st.name + "  "
            + TimeUtils::minutesToTimeString(st.startMinute, use24h);
        const QString prev = item->text();
        item->setText(prev.isEmpty() ? entry : prev + "\n" + entry);
    }
}
