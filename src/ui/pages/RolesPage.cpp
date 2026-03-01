#include "RolesPage.h"
#include "ui/dialogs/RoleDialog.h"
#include "domain/repositories/RoleRepository.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QPushButton>
#include <QMessageBox>
#include <QColor>
#include <QBrush>

RolesPage::RolesPage(QWidget* parent)
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
    m_table = new QTableWidget(0, 2, this);
    m_table->setHorizontalHeaderLabels({"Name", "Colour"});
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);

    auto* hdr = m_table->horizontalHeader();
    hdr->setSectionResizeMode(0, QHeaderView::Stretch);
    hdr->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    layout->addWidget(m_table);

    // Connections
    connect(m_table, &QTableWidget::itemSelectionChanged, this, [editBtn, deleteBtn, this]() {
        const bool hasSel = !m_table->selectedItems().isEmpty();
        editBtn->setEnabled(hasSel);
        deleteBtn->setEnabled(hasSel);
    });
    connect(addBtn,    &QPushButton::clicked, this, &RolesPage::onAddClicked);
    connect(editBtn,   &QPushButton::clicked, this, &RolesPage::onEditClicked);
    connect(deleteBtn, &QPushButton::clicked, this, &RolesPage::onDeleteClicked);
    connect(m_table,   &QTableWidget::cellDoubleClicked,
            this, &RolesPage::onCellDoubleClicked);

    refresh();
}

void RolesPage::refresh()
{
    m_table->setRowCount(0);
    m_ids.clear();

    RoleRepository repo;
    for (const Role& r : repo.getAll()) {
        const int row = m_table->rowCount();
        m_table->insertRow(row);
        m_ids.append(r.id);

        m_table->setItem(row, 0, new QTableWidgetItem(r.name));

        auto* colourItem = new QTableWidgetItem(r.colourHex);
        const QColor c(r.colourHex);
        if (c.isValid()) {
            colourItem->setBackground(QBrush(c));
            // Contrasting text
            const int brightness = c.red() * 299 + c.green() * 587 + c.blue() * 114;
            colourItem->setForeground(QBrush(QColor(brightness > 128000 ? "#000000" : "#ffffff")));
        }
        m_table->setItem(row, 1, colourItem);
    }
}

void RolesPage::onAddClicked()
{
    RoleDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted)
        refresh();
}

void RolesPage::onEditClicked()
{
    const int row = m_table->currentRow();
    if (row < 0 || row >= m_ids.size()) return;

    RoleRepository repo;
    auto opt = repo.getById(m_ids[row]);
    if (!opt) return;

    RoleDialog dlg(*opt, this);
    if (dlg.exec() == QDialog::Accepted)
        refresh();
}

void RolesPage::onDeleteClicked()
{
    const int row = m_table->currentRow();
    if (row < 0 || row >= m_ids.size()) return;

    const auto reply = QMessageBox::question(
        this, "Delete Role",
        "Are you sure you want to delete this role?\n"
        "This will also remove it from all employees and shift templates.",
        QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    RoleRepository repo;
    if (!repo.remove(m_ids[row]))
        QMessageBox::warning(this, "Cannot Delete",
            "Cannot delete: role is referenced by a shift template.");

    refresh();
}

void RolesPage::onCellDoubleClicked(int row, int /*col*/)
{
    if (row < 0 || row >= m_ids.size()) return;

    RoleRepository repo;
    auto opt = repo.getById(m_ids[row]);
    if (!opt) return;

    RoleDialog dlg(*opt, this);
    if (dlg.exec() == QDialog::Accepted)
        refresh();
}
