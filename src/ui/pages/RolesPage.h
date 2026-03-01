#pragma once
#include <QWidget>
#include <QVector>

class QTableWidget;

class RolesPage : public QWidget {
    Q_OBJECT
public:
    explicit RolesPage(QWidget* parent = nullptr);

    void refresh();

private slots:
    void onAddClicked();
    void onEditClicked();
    void onDeleteClicked();
    void onCellDoubleClicked(int row, int col);

private:
    QTableWidget* m_table = nullptr;
    QVector<int>  m_ids;
};
