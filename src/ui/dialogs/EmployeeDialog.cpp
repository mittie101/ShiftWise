#include "EmployeeDialog.h"
#include "domain/TimeUtils.h"
#include "domain/repositories/RoleRepository.h"
#include "domain/repositories/EmployeeRepository.h"
#include "domain/repositories/AvailabilityRepository.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QCheckBox>
#include <QTimeEdit>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QLabel>
#include <QTime>

EmployeeDialog::EmployeeDialog(QWidget* parent)
    : QDialog(parent), m_editMode(false)
{
    setWindowTitle("Add Employee");
    init();
    populateRoles();
}

EmployeeDialog::EmployeeDialog(const Employee& emp, QWidget* parent)
    : QDialog(parent), m_employee(emp), m_editMode(true)
{
    setWindowTitle("Edit Employee");
    init();

    m_nameEdit->setText(emp.name);
    m_maxHoursSpin->setValue(emp.maxWeeklyMinutes / 60);
    m_prioritySpin->setValue(emp.priority);

    populateRoles();
    loadAvailability();
}

void EmployeeDialog::init()
{
    setMinimumSize(420, 560);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);

    // ── General ───────────────────────────────────────────────────────────────
    auto* genGroup = new QGroupBox("General");
    auto* genForm  = new QFormLayout(genGroup);
    genForm->setSpacing(8);

    m_nameEdit = new QLineEdit;
    genForm->addRow("Name:", m_nameEdit);

    m_maxHoursSpin = new QSpinBox;
    m_maxHoursSpin->setRange(1, 168);
    m_maxHoursSpin->setSuffix(" h/week");
    m_maxHoursSpin->setValue(40);
    genForm->addRow("Max weekly hours:", m_maxHoursSpin);

    m_prioritySpin = new QSpinBox;
    m_prioritySpin->setRange(0, 10);
    m_prioritySpin->setToolTip(
        "Higher priority employees are scheduled first (0 = lowest, 10 = highest).\n"
        "Among equal priority, those with fewer scheduled hours are preferred.");
    genForm->addRow("Priority:", m_prioritySpin);

    mainLayout->addWidget(genGroup);

    // ── Roles ─────────────────────────────────────────────────────────────────
    auto* rolesGroup  = new QGroupBox("Roles");
    auto* rolesLayout = new QVBoxLayout(rolesGroup);
    m_rolesWidget = new QListWidget;
    m_rolesWidget->setMaximumHeight(120);
    rolesLayout->addWidget(m_rolesWidget);
    mainLayout->addWidget(rolesGroup);

    // ── Availability ──────────────────────────────────────────────────────────
    auto* availGroup = new QGroupBox("Availability");
    auto* grid       = new QGridLayout(availGroup);
    grid->setSpacing(6);

    grid->addWidget(new QLabel("Day"),   0, 0);
    grid->addWidget(new QLabel("Start"), 0, 1);
    grid->addWidget(new QLabel("End"),   0, 2);

    for (int d = 0; d < 7; ++d) {
        m_availCheck[d] = new QCheckBox(TimeUtils::dayOfWeekName(d));
        m_availStart[d] = new QTimeEdit(QTime(9, 0));
        m_availEnd[d]   = new QTimeEdit(QTime(17, 0));
        m_availStart[d]->setDisplayFormat("HH:mm");
        m_availEnd[d]->setDisplayFormat("HH:mm");
        m_availStart[d]->setEnabled(false);
        m_availEnd[d]->setEnabled(false);

        connect(m_availCheck[d], &QCheckBox::toggled, m_availStart[d], &QTimeEdit::setEnabled);
        connect(m_availCheck[d], &QCheckBox::toggled, m_availEnd[d],   &QTimeEdit::setEnabled);

        grid->addWidget(m_availCheck[d], d + 1, 0);
        grid->addWidget(m_availStart[d], d + 1, 1);
        grid->addWidget(m_availEnd[d],   d + 1, 2);
    }

    mainLayout->addWidget(availGroup);

    // ── Buttons ───────────────────────────────────────────────────────────────
    auto* btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    mainLayout->addWidget(btns);

    connect(btns, &QDialogButtonBox::accepted, this, &EmployeeDialog::onAccepted);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void EmployeeDialog::populateRoles()
{
    m_rolesWidget->clear();

    RoleRepository roleRepo;
    const QVector<Role> allRoles = roleRepo.getAll();

    QVector<int> checkedIds;
    if (m_editMode && m_employee.id != -1) {
        EmployeeRepository empRepo;
        for (const Role& r : empRepo.getRolesForEmployee(m_employee.id))
            checkedIds.append(r.id);
    }

    for (const Role& r : allRoles) {
        auto* item = new QListWidgetItem(r.name, m_rolesWidget);
        item->setData(Qt::UserRole, r.id);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(checkedIds.contains(r.id) ? Qt::Checked : Qt::Unchecked);
    }
}

void EmployeeDialog::loadAvailability()
{
    if (!m_editMode || m_employee.id == -1)
        return;

    AvailabilityRepository availRepo;
    for (const Availability& av : availRepo.getForEmployee(m_employee.id)) {
        const int d = av.dayOfWeek;
        if (d < 0 || d > 6) continue;
        m_availCheck[d]->setChecked(true);
        m_availStart[d]->setTime(QTime(av.startMinute / 60, av.startMinute % 60));
        m_availEnd[d]->setTime(QTime(av.endMinute   / 60, av.endMinute   % 60));
    }
}

void EmployeeDialog::onAccepted()
{
    if (m_nameEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Validation", "Name cannot be empty.");
        return;
    }

    // Duplicate name check (case-insensitive, skip self in edit mode)
    const QString trimmedName = m_nameEdit->text().trimmed();
    EmployeeRepository checkRepo;
    for (const Employee& e : checkRepo.getAll()) {
        if (m_editMode && e.id == m_employee.id) continue;
        if (e.name.compare(trimmedName, Qt::CaseInsensitive) == 0) {
            QMessageBox::warning(this, "Validation",
                QString("An employee named \"%1\" already exists.").arg(trimmedName));
            return;
        }
    }

    m_employee.name             = m_nameEdit->text().trimmed();
    m_employee.maxWeeklyMinutes = m_maxHoursSpin->value() * 60;
    m_employee.priority         = m_prioritySpin->value();

    EmployeeRepository empRepo;
    if (m_editMode) {
        empRepo.update(m_employee);
        m_savedId = m_employee.id;
    } else {
        m_savedId     = empRepo.insert(m_employee);
        m_employee.id = m_savedId;
    }

    if (m_savedId == -1) {
        QMessageBox::critical(this, "Error", "Failed to save employee.");
        return;
    }

    // Collect checked role ids
    QVector<int> roleIds;
    for (int i = 0; i < m_rolesWidget->count(); ++i) {
        const QListWidgetItem* item = m_rolesWidget->item(i);
        if (item->checkState() == Qt::Checked)
            roleIds.append(item->data(Qt::UserRole).toInt());
    }
    empRepo.setRolesForEmployee(m_savedId, roleIds);

    // Validate availability windows (start must differ from end)
    for (int d = 0; d < 7; ++d) {
        if (!m_availCheck[d]->isChecked()) continue;
        const int s = m_availStart[d]->time().hour() * 60 + m_availStart[d]->time().minute();
        const int e = m_availEnd[d]->time().hour()   * 60 + m_availEnd[d]->time().minute();
        if (s == e) {
            QMessageBox::warning(this, "Validation",
                QString("%1 availability has the same start and end time.\n"
                        "Please set a valid time range.")
                    .arg(TimeUtils::dayOfWeekName(d)));
            return;
        }
    }

    // Collect enabled availability rows
    QVector<Availability> availList;
    for (int d = 0; d < 7; ++d) {
        if (!m_availCheck[d]->isChecked()) continue;
        Availability av;
        av.employeeId  = m_savedId;
        av.dayOfWeek   = d;
        av.startMinute = m_availStart[d]->time().hour() * 60 + m_availStart[d]->time().minute();
        av.endMinute   = m_availEnd[d]->time().hour()   * 60 + m_availEnd[d]->time().minute();
        availList.append(av);
    }
    AvailabilityRepository availRepo;
    availRepo.replaceForEmployee(m_savedId, availList);

    accept();
}

int EmployeeDialog::savedId() const
{
    return m_savedId;
}
