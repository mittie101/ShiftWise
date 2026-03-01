#pragma once
#include <QDialog>
#include <QColor>
#include "domain/models/Role.h"

class QLineEdit;
class QPushButton;

class RoleDialog : public QDialog {
    Q_OBJECT
public:
    explicit RoleDialog(QWidget* parent = nullptr);                  // add mode
    explicit RoleDialog(const Role& role, QWidget* parent = nullptr); // edit mode

    Role role() const;

private slots:
    void onPickColour();
    void onAccepted();

private:
    void init();
    void applyColourToButton();

    QLineEdit*   m_nameEdit   = nullptr;
    QPushButton* m_colourBtn  = nullptr;

    Role   m_result;
    QColor m_colour;
    bool   m_editMode = false;
};
