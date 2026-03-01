#pragma once
#include <QDialog>
#include "domain/models/Employee.h"

class QLineEdit;
class QSpinBox;
class QListWidget;
class QCheckBox;
class QTimeEdit;

class EmployeeDialog : public QDialog {
    Q_OBJECT
public:
    explicit EmployeeDialog(QWidget* parent = nullptr);                      // add mode
    explicit EmployeeDialog(const Employee& emp, QWidget* parent = nullptr); // edit mode

    int savedId() const;

private slots:
    void onAccepted();

private:
    void init();
    void populateRoles();
    void loadAvailability();

    // General
    QLineEdit* m_nameEdit     = nullptr;
    QSpinBox*  m_maxHoursSpin = nullptr;
    QSpinBox*  m_prioritySpin = nullptr;

    // Roles
    QListWidget* m_rolesWidget = nullptr;

    // Availability (one entry per day 0=Mon..6=Sun)
    QCheckBox* m_availCheck[7] = {};
    QTimeEdit* m_availStart[7] = {};
    QTimeEdit* m_availEnd[7]   = {};

    Employee m_employee;
    bool     m_editMode = false;
    int      m_savedId  = -1;
};
