#pragma once
#include <QMainWindow>
#include <QStackedWidget>
#include <QLabel>
#include <QPushButton>
#include <QVector>

class QToolBar;
class SchedulePage;
class ReportsPage;

// Page indices — must match order added to m_stack
enum class Page {
    Roles     = 0,
    Employees = 1,
    Shifts    = 2,
    Schedule  = 3,
    Reports   = 4,
    Settings  = 5,
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

public slots:
    void onGenerateClicked();
    void onClearUnlockedClicked();
    void onExportCsvClicked();
    void onPrevWeekClicked();
    void onNextWeekClicked();

private:
    void buildSidebar();
    void buildToolbar();
    void buildPages();
    void navigateTo(Page page);
    void updateWeekLabel();
    QString currentWeekStart() const;   // ISO date, always Monday

    QWidget*       m_sidebar       = nullptr;
    QStackedWidget* m_stack        = nullptr;
    QLabel*        m_weekLabel     = nullptr;
    QLabel*        m_saveIndicator = nullptr;
    QVector<QPushButton*> m_navBtns;

    QString m_currentWeekStart;         // ISO date of displayed Monday

    SchedulePage* m_schedulePage = nullptr;
    ReportsPage*  m_reportsPage  = nullptr;
};
