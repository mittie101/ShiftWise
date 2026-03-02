#pragma once
#include <QWidget>
#include <QString>

class QTableWidget;
class QLabel;

class ReportsPage : public QWidget {
    Q_OBJECT
public:
    explicit ReportsPage(QWidget* parent = nullptr);

public slots:
    void loadWeek(const QString& weekStart);

private slots:
    void onExportCsvClicked();

private:
    QString       m_weekStart;
    QLabel*       m_summaryLabel   = nullptr;
    QTableWidget* m_table          = nullptr;
    QTableWidget* m_unfilledTable  = nullptr;
};
