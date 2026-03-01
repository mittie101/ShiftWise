#include "SchedulePage.h"
#include "app/AppSettings.h"
#include "domain/TimeUtils.h"
#include "domain/repositories/ScheduleRepository.h"
#include "domain/repositories/ShiftTemplateRepository.h"
#include "domain/repositories/EmployeeRepository.h"
#include "domain/repositories/RoleRepository.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QPushButton>
#include <QCheckBox>
#include <QDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QHash>

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

    m_table = new QTableWidget(0, 7, this);
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

    layout->addWidget(m_table);
}

void SchedulePage::loadWeek(const QString& weekStart)
{
    m_weekStart = weekStart;
    m_table->setRowCount(0);
    m_rows.clear();
    m_assignments.clear();

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

    QHash<int, QString> roleNames;
    for (const Role& r : roleRepo.getAll())
        roleNames.insert(r.id, r.name);

    const bool use24h = AppSettings::instance().use24HourFormat();

    for (const ShiftTemplate& st : shiftRepo.getAll()) {
        const QString dayStr  = TimeUtils::dayOfWeekName(st.dayOfWeek);
        const QString timeStr = TimeUtils::minutesToTimeString(st.startMinute, use24h)
            + QString::fromUtf8(" \xe2\x80\x93 ")       // " – "
            + TimeUtils::minutesToTimeString(st.endMinute, use24h);
        const QString roleStr = roleNames.value(st.roleId, QString("(id %1)").arg(st.roleId));

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
            m_table->setItem(row, kColRole,  new QTableWidgetItem(roleStr));
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
}

void SchedulePage::onAssignClicked(int row)
{
    if (row < 0 || row >= m_rows.size()) return;
    const RowMeta meta = m_rows[row];

    // Build inline assignment dialog
    EmployeeRepository empRepo;
    const QVector<Employee> employees = empRepo.getAll();

    QDialog dlg(this);
    dlg.setWindowTitle("Assign Employee");
    auto* layout = new QVBoxLayout(&dlg);

    auto* combo = new QComboBox;
    combo->addItem(QString::fromUtf8("\xe2\x80\x94 unassign \xe2\x80\x94"), -1);
    for (const Employee& e : employees)
        combo->addItem(e.name, e.id);

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
}

void SchedulePage::onLockToggled(int row, bool locked)
{
    if (row < 0 || row >= m_rows.size()) return;
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
}
