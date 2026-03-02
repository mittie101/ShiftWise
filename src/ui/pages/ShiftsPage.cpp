#include "ShiftsPage.h"
#include "ui/dialogs/ShiftTemplateDialog.h"
#include "app/AppSettings.h"
#include "domain/TimeUtils.h"
#include "domain/repositories/ShiftTemplateRepository.h"
#include "domain/repositories/RoleRepository.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QPushButton>
#include <QMessageBox>
#include <QHash>
#include <algorithm>

ShiftsPage::ShiftsPage(QWidget* parent)
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
    m_table = new QTableWidget(0, 6, this);
    m_table->setHorizontalHeaderLabels({
        "Day", "Name", "Start", "End", "Role", "Staff Required"
    });
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);

    auto* hdr = m_table->horizontalHeader();
    hdr->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    hdr->setSectionResizeMode(1, QHeaderView::Stretch);
    hdr->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    hdr->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    hdr->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    hdr->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    layout->addWidget(m_table);

    // Connections
    connect(m_table, &QTableWidget::itemSelectionChanged, this, [editBtn, deleteBtn, this]() {
        const bool hasSel = !m_table->selectedItems().isEmpty();
        editBtn->setEnabled(hasSel);
        deleteBtn->setEnabled(hasSel);
    });
    connect(addBtn,    &QPushButton::clicked, this, &ShiftsPage::onAddClicked);
    connect(editBtn,   &QPushButton::clicked, this, &ShiftsPage::onEditClicked);
    connect(deleteBtn, &QPushButton::clicked, this, &ShiftsPage::onDeleteClicked);
    connect(m_table,   &QTableWidget::cellDoubleClicked,
            this, &ShiftsPage::onCellDoubleClicked);

    refresh();
}

void ShiftsPage::refresh()
{
    m_table->setRowCount(0);
    m_ids.clear();

    ShiftTemplateRepository stRepo;
    RoleRepository          roleRepo;

    QHash<int, QString> roleNames;
    for (const Role& r : roleRepo.getAll())
        roleNames.insert(r.id, r.name);

    const bool use24h = AppSettings::instance().use24HourFormat();

    QVector<ShiftTemplate> shifts = stRepo.getAll();
    std::sort(shifts.begin(), shifts.end(), [](const ShiftTemplate& a, const ShiftTemplate& b) {
        if (a.dayOfWeek != b.dayOfWeek) return a.dayOfWeek < b.dayOfWeek;
        return a.startMinute < b.startMinute;
    });

    for (const ShiftTemplate& st : shifts) {
        const int row = m_table->rowCount();
        m_table->insertRow(row);
        m_ids.append(st.id);

        m_table->setItem(row, 0, new QTableWidgetItem(TimeUtils::dayOfWeekName(st.dayOfWeek)));
        m_table->setItem(row, 1, new QTableWidgetItem(st.name));
        m_table->setItem(row, 2, new QTableWidgetItem(
            TimeUtils::minutesToTimeString(st.startMinute, use24h)));
        m_table->setItem(row, 3, new QTableWidgetItem(
            TimeUtils::minutesToTimeString(st.endMinute, use24h)));
        m_table->setItem(row, 4, new QTableWidgetItem(
            roleNames.value(st.roleId, QString("(id %1)").arg(st.roleId))));
        m_table->setItem(row, 5, new QTableWidgetItem(QString::number(st.staffRequired)));
    }
}

void ShiftsPage::onAddClicked()
{
    ShiftTemplateDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted)
        refresh();
}

void ShiftsPage::onEditClicked()
{
    const int row = m_table->currentRow();
    if (row < 0 || row >= m_ids.size()) return;

    ShiftTemplateRepository repo;
    auto opt = repo.getById(m_ids[row]);
    if (!opt) return;

    ShiftTemplateDialog dlg(*opt, this);
    if (dlg.exec() == QDialog::Accepted)
        refresh();
}

void ShiftsPage::onDeleteClicked()
{
    const int row = m_table->currentRow();
    if (row < 0 || row >= m_ids.size()) return;

    const auto reply = QMessageBox::question(
        this, "Delete Shift",
        "Are you sure you want to delete this shift template?",
        QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    ShiftTemplateRepository repo;
    if (!repo.remove(m_ids[row]))
        QMessageBox::warning(this, "Cannot Delete",
            "Cannot delete: shift has assignments.");

    refresh();
}

void ShiftsPage::onCellDoubleClicked(int row, int /*col*/)
{
    if (row < 0 || row >= m_ids.size()) return;

    ShiftTemplateRepository repo;
    auto opt = repo.getById(m_ids[row]);
    if (!opt) return;

    ShiftTemplateDialog dlg(*opt, this);
    if (dlg.exec() == QDialog::Accepted)
        refresh();
}
