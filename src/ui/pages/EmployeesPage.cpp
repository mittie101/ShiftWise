#include "EmployeesPage.h"
#include "ui/dialogs/EmployeeDialog.h"
#include "domain/repositories/EmployeeRepository.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QPushButton>
#include <QMessageBox>

EmployeesPage::EmployeesPage(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(8);

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
    m_table = new QTableWidget(0, 3, this);
    m_table->setHorizontalHeaderLabels({"Name", "Max Hours/Week", "Priority"});
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);

    auto* hdr = m_table->horizontalHeader();
    hdr->setSectionResizeMode(0, QHeaderView::Stretch);
    hdr->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    hdr->setSectionResizeMode(2, QHeaderView::ResizeToContents);
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

        m_table->setItem(row, 0, new QTableWidgetItem(e.name));
        m_table->setItem(row, 1, new QTableWidgetItem(
            QString("%1 h").arg(e.maxWeeklyMinutes / 60)));
        m_table->setItem(row, 2, new QTableWidgetItem(QString::number(e.priority)));
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

    const auto reply = QMessageBox::question(
        this, "Delete Employee",
        "Are you sure you want to delete this employee?",
        QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    EmployeeRepository repo;
    if (!repo.remove(m_ids[row]))
        QMessageBox::warning(this, "Cannot Delete",
            "Cannot delete: employee has scheduled assignments.");

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
