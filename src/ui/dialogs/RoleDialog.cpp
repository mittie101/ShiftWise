#include "RoleDialog.h"
#include "domain/repositories/RoleRepository.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QColorDialog>
#include <QMessageBox>

RoleDialog::RoleDialog(QWidget* parent)
    : QDialog(parent), m_editMode(false)
{
    m_colour = QColor(QStringLiteral("#e94560")); // default colour
    setWindowTitle("Add Role");
    init();
}

RoleDialog::RoleDialog(const Role& role, QWidget* parent)
    : QDialog(parent), m_result(role), m_editMode(true)
{
    m_colour = QColor(role.colourHex);
    if (!m_colour.isValid())
        m_colour = QColor(QStringLiteral("#e94560"));
    setWindowTitle("Edit Role");
    init();
    m_nameEdit->setText(role.name);
}

void RoleDialog::init()
{
    setMinimumWidth(280);

    auto* form = new QFormLayout;
    form->setSpacing(10);

    m_nameEdit = new QLineEdit;
    form->addRow("Name:", m_nameEdit);

    m_colourBtn = new QPushButton;
    m_colourBtn->setMinimumWidth(80);
    applyColourToButton();
    form->addRow("Colour:", m_colourBtn);

    auto* btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(btns);

    connect(m_colourBtn, &QPushButton::clicked, this, &RoleDialog::onPickColour);
    connect(btns, &QDialogButtonBox::accepted, this, &RoleDialog::onAccepted);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void RoleDialog::applyColourToButton()
{
    const QString hex = m_colour.name();
    // Choose contrasting text colour for readability
    const int brightness = m_colour.red() * 299 + m_colour.green() * 587 + m_colour.blue() * 114;
    const QString fg = (brightness > 128000) ? "#000000" : "#ffffff";
    m_colourBtn->setText(hex);
    m_colourBtn->setStyleSheet(
        QString("QPushButton { background-color: %1; color: %2; border-radius: 3px; padding: 4px 8px; }")
            .arg(hex, fg));
}

void RoleDialog::onPickColour()
{
    const QColor chosen = QColorDialog::getColor(m_colour, this, "Pick Role Colour");
    if (chosen.isValid()) {
        m_colour = chosen;
        applyColourToButton();
    }
}

void RoleDialog::onAccepted()
{
    if (m_nameEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Validation", "Name cannot be empty.");
        return;
    }

    m_result.name      = m_nameEdit->text().trimmed();
    m_result.colourHex = m_colour.name();

    RoleRepository repo;
    if (m_editMode)
        repo.update(m_result);
    else
        m_result.id = repo.insert(m_result);

    accept();
}

Role RoleDialog::role() const
{
    return m_result;
}
