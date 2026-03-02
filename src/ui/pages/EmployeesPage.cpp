#include "EmployeesPage.h"
#include "ui/dialogs/EmployeeDialog.h"
#include "domain/repositories/EmployeeRepository.h"
#include "domain/repositories/ScheduleRepository.h"
#include "domain/models/Role.h"
#include "domain/models/Schedule.h"
#include "domain/models/Assignment.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QPushButton>
#include <QLineEdit>
#include <QMessageBox>

EmployeesPage::EmployeesPage(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(8);

    // Search bar
    m_searchEdit = new QLineEdit;
    m_searchEdit->setPlaceholderText("Search employees\xe2\x80\xa6");  // "Search employees…"
    m_searchEdit->setClearButtonEnabled(true);
    layout->addWidget(m_searchEdit);

    // Button row
    auto* btnRow    = new QHBoxLayout;
    auto* addBtn    = new QPushButton("Add");
    auto* editBtn   = new QPushButton("Edit");
    auto* deleteBtn = new QPushButton("Delete");
    editBtn->setEnabled(false);
    deleteBtn->setEnabled(false);
    btnRow->addWidget(addBtn);
    btnRow->addWidget(editBtn);
    btnRow->addWidget(deleteBtn);
    btnRow->addStretch();
    layout->addLayout(btnRow);

    // Table
    m_table = new QTableWidget(0, 4, this);
    m_table->setHorizontalHeaderLabels({"Name", "Roles", "Max Hours/Week", "Priority"});
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);

    auto* hdr = m_table->horizontalHeader();
    hdr->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    hdr->setSectionResizeMode(1, QHeaderView::Stretch);
    hdr->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    hdr->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    layout->addWidget(m_table);

    // Connections
    connect(m_table, &QTableWidget::itemSelectionChanged, this, [editBtn, deleteBtn, this]() {
        const bool hasSel = !m_table->selectedItems().isEmpty();
        editBtn->setEnabled(hasSel);
        deleteBtn->setEnabled(hasSel);
    });
    connect(addBtn,    &QPushButton::clicked, this, &EmployeesPage::onAddClicked);
    connect(editBtn,   &QPushButton::clicked, this, &EmployeesPage::onEditClicked);
    connect(deleteBtn, &QPushButton::clicked, this, &EmployeesPage::onDeleteClicked);
    connect(m_table,   &QTableWidget::cellDoubleClicked,
            this, &EmployeesPage::onCellDoubleClicked);
    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &EmployeesPage::applyFilter);

    refresh();
}

void EmployeesPage::refresh()
{
    m_table->setRowCount(0);
    m_ids.clear();

    EmployeeRepository repo;
    for (const Employee& e : repo.getAll()) {
        const int row = m_table->rowCount();
        m_table->insertRow(row);
        m_ids.append(e.id);

        QStringList roleNames;
        for (const Role& r : repo.getRolesForEmployee(e.id))
            roleNames << r.name;

        m_table->setItem(row, 0, new QTableWidgetItem(e.name));
        m_table->setItem(row, 1, new QTableWidgetItem(roleNames.join(", ")));
        m_table->setItem(row, 2, new QTableWidgetItem(
            QString("%1 h").arg(e.maxWeeklyMinutes / 60)));
        m_table->setItem(row, 3, new QTableWidgetItem(QString::number(e.priority)));
    }

    applyFilter();
}

void EmployeesPage::applyFilter()
{
    const QString text = m_searchEdit ? m_searchEdit->text().trimmed() : QString();
    for (int r = 0; r < m_table->rowCount(); ++r) {
        const auto* item = m_table->item(r, 0);
        const bool match = text.isEmpty()
            || (item && item->text().contains(text, Qt::CaseInsensitive));
        m_table->setRowHidden(r, !match);
    }
}

void EmployeesPage::onAddClicked()
{
    EmployeeDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted)
        refresh();
}

void EmployeesPage::onEditClicked()
{
    const int row = m_table->currentRow();
    if (row < 0 || row >= m_ids.size()) return;

    EmployeeRepository repo;
    auto opt = repo.getById(m_ids[row]);
    if (!opt) return;

    EmployeeDialog dlg(*opt, this);
    if (dlg.exec() == QDialog::Accepted)
        refresh();
}

void EmployeesPage::onDeleteClicked()
{
    const int row = m_table->currentRow();
    if (row < 0 || row >= m_ids.size()) return;

    // Count existing assignments for this employee across all weeks
    ScheduleRepository schedRepo;
    int assignCount = 0;
    for (const Schedule& s : schedRepo.getAll())
        for (const Assignment& a : schedRepo.getAssignments(s.id))
            if (a.employeeId == m_ids[row]) ++assignCount;

    if (assignCount > 0) {
        QMessageBox::warning(this, "Cannot Delete Employee",
            QString("This employee has %1 assignment(s) across the schedule.\n\n"
                    "Remove or reassign their shifts before deleting.")
                .arg(assignCount));
        return;
    }

    const auto reply = QMessageBox::question(
        this, "Delete Employee",
        "Are you sure you want to delete this employee?",
        QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    EmployeeRepository repo;
    repo.remove(m_ids[row]);
    refresh();
}

void EmployeesPage::onCellDoubleClicked(int row, int /*col*/)
{
    if (row < 0 || row >= m_ids.size()) return;

    EmployeeRepository repo;
    auto opt = repo.getById(m_ids[row]);
    if (!opt) return;

    EmployeeDialog dlg(*opt, this);
    if (dlg.exec() == QDialog::Accepted)
        refresh();
}
