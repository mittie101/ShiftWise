#include "ShiftTemplateDialog.h"
#include "domain/TimeUtils.h"
#include "domain/repositories/RoleRepository.h"
#include "domain/repositories/ShiftTemplateRepository.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QComboBox>
#include <QTimeEdit>
#include <QSpinBox>
#include <QMessageBox>
#include <QTime>

ShiftTemplateDialog::ShiftTemplateDialog(QWidget* parent)
    : QDialog(parent), m_editMode(false)
{
    setWindowTitle("Add Shift Template");
    init();
}

ShiftTemplateDialog::ShiftTemplateDialog(const ShiftTemplate& st, QWidget* parent)
    : QDialog(parent), m_result(st), m_editMode(true)
{
    setWindowTitle("Edit Shift Template");
    init();

    // Pre-fill fields from existing template
    m_nameEdit->setText(st.name);
    m_dayCombo->setCurrentIndex(st.dayOfWeek);
    m_startEdit->setTime(QTime(st.startMinute / 60, st.startMinute % 60));
    m_endEdit->setTime(QTime(st.endMinute / 60, st.endMinute % 60));
    m_staffSpin->setValue(st.staffRequired);

    for (int i = 0; i < m_roleCombo->count(); ++i) {
        if (m_roleCombo->itemData(i).toInt() == st.roleId) {
            m_roleCombo->setCurrentIndex(i);
            break;
        }
    }
}

void ShiftTemplateDialog::init()
{
    setMinimumWidth(320);

    auto* form = new QFormLayout;
    form->setSpacing(10);

    m_nameEdit = new QLineEdit;
    form->addRow("Name:", m_nameEdit);

    m_dayCombo = new QComboBox;
    for (int i = 0; i < 7; ++i)
        m_dayCombo->addItem(TimeUtils::dayOfWeekName(i));
    form->addRow("Day:", m_dayCombo);

    m_startEdit = new QTimeEdit(QTime(9, 0));
    m_startEdit->setDisplayFormat("HH:mm");
    form->addRow("Start:", m_startEdit);

    m_endEdit = new QTimeEdit(QTime(17, 0));
    m_endEdit->setDisplayFormat("HH:mm");
    form->addRow("End:", m_endEdit);

    m_roleCombo = new QComboBox;
    populateRoles();
    form->addRow("Role:", m_roleCombo);

    m_staffSpin = new QSpinBox;
    m_staffSpin->setRange(1, 20);
    form->addRow("Staff required:", m_staffSpin);

    auto* btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(btns);

    connect(btns, &QDialogButtonBox::accepted, this, &ShiftTemplateDialog::onAccepted);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void ShiftTemplateDialog::populateRoles()
{
    RoleRepository repo;
    for (const Role& r : repo.getAll())
        m_roleCombo->addItem(r.name, r.id);
}

void ShiftTemplateDialog::onAccepted()
{
    if (m_nameEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Validation", "Name cannot be empty.");
        return;
    }
    if (m_roleCombo->count() == 0) {
        QMessageBox::warning(this, "Validation",
            "No roles available. Please add a role before creating a shift template.");
        return;
    }

    m_result.name          = m_nameEdit->text().trimmed();
    m_result.dayOfWeek     = m_dayCombo->currentIndex();
    m_result.startMinute   = m_startEdit->time().hour() * 60 + m_startEdit->time().minute();
    m_result.endMinute     = m_endEdit->time().hour()   * 60 + m_endEdit->time().minute();
    m_result.roleId        = m_roleCombo->currentData().toInt();
    m_result.staffRequired = m_staffSpin->value();

    ShiftTemplateRepository repo;
    if (m_editMode)
        repo.update(m_result);
    else
        m_result.id = repo.insert(m_result);

    accept();
}

ShiftTemplate ShiftTemplateDialog::shiftTemplate() const
{
    return m_result;
}
