#pragma once
#include <QWidget>

class QTableWidget;

class ReportsPage : public QWidget {
    Q_OBJECT
public:
    explicit ReportsPage(QWidget* parent = nullptr);

public slots:
    void loadWeek(const QString& weekStart);

private:
    QTableWidget* m_table = nullptr;
};
