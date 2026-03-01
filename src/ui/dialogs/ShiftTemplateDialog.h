#pragma once
#include <QDialog>
#include "domain/models/ShiftTemplate.h"

class QLineEdit;
class QComboBox;
class QTimeEdit;
class QSpinBox;

class ShiftTemplateDialog : public QDialog {
    Q_OBJECT
public:
    explicit ShiftTemplateDialog(QWidget* parent = nullptr);                         // add mode
    explicit ShiftTemplateDialog(const ShiftTemplate& st, QWidget* parent = nullptr); // edit mode

    ShiftTemplate shiftTemplate() const;

private slots:
    void onAccepted();

private:
    void init();
    void populateRoles();

    QLineEdit* m_nameEdit  = nullptr;
    QComboBox* m_dayCombo  = nullptr;
    QTimeEdit* m_startEdit = nullptr;
    QTimeEdit* m_endEdit   = nullptr;
    QComboBox* m_roleCombo = nullptr;
    QSpinBox*  m_staffSpin = nullptr;

    ShiftTemplate m_result;
    bool          m_editMode = false;
};
