#pragma once
#include <QWidget>
#include <QVector>

class QTableWidget;
class QLineEdit;

class EmployeesPage : public QWidget {
    Q_OBJECT
public:
    explicit EmployeesPage(QWidget* parent = nullptr);

    void refresh();

private:
    void applyFilter();

private slots:
    void onAddClicked();
    void onEditClicked();
    void onDeleteClicked();
    void onCellDoubleClicked(int row, int col);

private:
    QLineEdit*    m_searchEdit = nullptr;
    QTableWidget* m_table      = nullptr;
    QVector<int>  m_ids;
};
