#pragma once
#include <QWidget>
#include <QVector>
#include <QMap>
#include <QHash>
#include <QPair>
#include "domain/models/Assignment.h"
#include "domain/models/ShiftTemplate.h"

class QTableWidget;
class QTabWidget;

class SchedulePage : public QWidget {
    Q_OBJECT
public:
    explicit SchedulePage(QWidget* parent = nullptr);

public slots:
    void loadWeek(const QString& weekStart);

signals:
    void saved();

private:
    void onAssignClicked(int row);
    void onLockToggled(int row, bool locked);
    void refreshCalendar();

    struct RowMeta {
        bool isHeader        = false;
        int  shiftTemplateId = -1;
        int  slotIndex       = -1;
        int  scheduleId      = -1;
    };

    QTabWidget*   m_tabs     = nullptr;
    QTableWidget* m_table    = nullptr;
    QTableWidget* m_calTable = nullptr;
    QVector<RowMeta> m_rows;
    QMap<QPair<int,int>, Assignment> m_assignments;
    QHash<int, ShiftTemplate> m_shiftMap;
    QString m_weekStart;
};
