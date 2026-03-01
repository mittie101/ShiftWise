#pragma once
#include <QWidget>
#include <QVector>
#include <QMap>
#include <QPair>
#include "domain/models/Assignment.h"

class QTableWidget;

class SchedulePage : public QWidget {
    Q_OBJECT
public:
    explicit SchedulePage(QWidget* parent = nullptr);

public slots:
    void loadWeek(const QString& weekStart);

private:
    void onAssignClicked(int row);
    void onLockToggled(int row, bool locked);

    struct RowMeta {
        int shiftTemplateId;
        int slotIndex;
        int scheduleId;
    };

    QTableWidget* m_table = nullptr;
    QVector<RowMeta> m_rows;
    QMap<QPair<int,int>, Assignment> m_assignments; // key: (shiftTemplateId, slotIndex)
    QString m_weekStart;
};
