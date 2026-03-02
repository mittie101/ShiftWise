#include "MainWindow.h"
#include "ui/pages/RolesPage.h"
#include "ui/pages/EmployeesPage.h"
#include "ui/pages/ShiftsPage.h"
#include "ui/pages/SchedulePage.h"
#include "ui/pages/ReportsPage.h"
#include "ui/pages/SettingsPage.h"
#include "app/AppSettings.h"
#include "domain/Scheduler.h"
#include "domain/TimeUtils.h"
#include "domain/repositories/ScheduleRepository.h"
#include "domain/repositories/ShiftTemplateRepository.h"
#include "domain/repositories/EmployeeRepository.h"
#include "domain/repositories/RoleRepository.h"
#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QToolBar>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QDate>
#include <QStatusBar>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QMessageBox>
#include <QHash>
#include <QCloseEvent>
#include <QPrinter>
#include <QPrintDialog>
#include <QTextDocument>
#include <QMenuBar>
#include <QTimer>
#include <QShortcut>

// ─────────────────────────────────────────────────────────────────────────────

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("ShiftWise");
    setMinimumSize(1100, 680);
    resize(1280, 780);

    // Restore last-viewed week (fall back to current week if none saved)
    const QString saved = AppSettings::instance().lastWeekStart();
    const QDate savedDate = QDate::fromString(saved, Qt::ISODate);
    if (savedDate.isValid() && savedDate.dayOfWeek() == 1) {
        m_currentWeekStart = saved;
    } else {
        QDate today = QDate::currentDate();
        int daysToMon = (today.dayOfWeek() - 1 + 7) % 7;
        m_currentWeekStart = today.addDays(-daysToMon).toString(Qt::ISODate);
    }

    // ── Central layout ────────────────────────────────────────────────────────
    QWidget* central = new QWidget(this);
    setCentralWidget(central);

    QHBoxLayout* mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    buildSidebar();
    mainLayout->addWidget(m_sidebar);

    m_stack = new QStackedWidget;
    mainLayout->addWidget(m_stack, 1);

    buildPages();
    buildToolbar();

    // ── Menu bar ──────────────────────────────────────────────────────────────
    QMenu* helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction("About ShiftWise", this, &MainWindow::onAboutClicked);

    // ── Auto-save indicator wiring ────────────────────────────────────────────
    connect(m_schedulePage, &SchedulePage::saved, this, &MainWindow::onScheduleSaved);

    // ── Status bar ────────────────────────────────────────────────────────────
    m_saveIndicator = new QLabel("● Saved");
    m_saveIndicator->setObjectName("saveIndicator");
    statusBar()->addPermanentWidget(m_saveIndicator);
    statusBar()->showMessage(QString("ShiftWise v%1").arg(SHIFTWISE_VERSION));

    navigateTo(Page::Schedule);
}

// ── Sidebar ──────────────────────────────────────────────────────────────────

struct NavItem { QString label; Page page; };

void MainWindow::buildSidebar() {
    m_sidebar = new QWidget;
    m_sidebar->setObjectName("sidebar");

    QVBoxLayout* vl = new QVBoxLayout(m_sidebar);
    vl->setContentsMargins(0, 0, 0, 0);
    vl->setSpacing(0);

    // Logo / app name
    QWidget* logoRow = new QWidget;
    logoRow->setObjectName("sidebarLogo");
    QVBoxLayout* ll = new QVBoxLayout(logoRow);
    ll->setContentsMargins(16, 18, 16, 14);
    QLabel* appName = new QLabel("ShiftWise");
    appName->setObjectName("sidebarLogo");
    QFont f = appName->font();
    f.setPointSize(16);
    f.setBold(true);
    appName->setFont(f);
    QLabel* version = new QLabel(QString("v%1").arg(SHIFTWISE_VERSION));
    version->setObjectName("mutedLabel");
    ll->addWidget(appName);
    ll->addWidget(version);
    vl->addWidget(logoRow);

    // Separator
    QFrame* sep = new QFrame;
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("background-color: #2a2a4a; max-height: 1px;");
    vl->addWidget(sep);
    vl->addSpacing(8);

    // Nav buttons
    const NavItem items[] = {
        { "  Roles",     Page::Roles     },
        { "  Employees", Page::Employees },
        { "  Shifts",    Page::Shifts    },
        { "  Schedule",  Page::Schedule  },
        { "  Reports",   Page::Reports   },
        { "  Settings",  Page::Settings  },
    };

    for (const auto& item : items) {
        QPushButton* btn = new QPushButton(item.label);
        btn->setObjectName("navBtn");
        btn->setCheckable(true);
        btn->setAutoExclusive(true);
        btn->setFixedHeight(40);

        const Page page = item.page;
        connect(btn, &QPushButton::clicked, this, [this, page]() {
            navigateTo(page);
        });

        m_navBtns.append(btn);
        vl->addWidget(btn);
    }

    vl->addStretch();

    // Bottom version note
    QLabel* bottomNote = new QLabel("Staff Scheduling");
    bottomNote->setObjectName("mutedLabel");
    bottomNote->setAlignment(Qt::AlignCenter);
    bottomNote->setContentsMargins(0, 0, 0, 12);
    vl->addWidget(bottomNote);
}

// ── Toolbar ───────────────────────────────────────────────────────────────────

void MainWindow::buildToolbar() {
    QToolBar* tb = new QToolBar("Main Toolbar", this);
    tb->setMovable(false);
    tb->setFloatable(false);
    addToolBar(Qt::TopToolBarArea, tb);

    // Week navigation
    QPushButton* prevBtn = new QPushButton("\xe2\x97\x80"); // ◀
    prevBtn->setObjectName("weekNavBtn");
    prevBtn->setToolTip("Previous week  Ctrl+\xe2\x86\x90");
    connect(prevBtn, &QPushButton::clicked, this, &MainWindow::onPrevWeekClicked);

    m_weekLabel = new QLabel;
    m_weekLabel->setMinimumWidth(180);
    m_weekLabel->setAlignment(Qt::AlignCenter);
    updateWeekLabel();

    QPushButton* nextBtn = new QPushButton("\xe2\x96\xb6"); // ▶
    nextBtn->setObjectName("weekNavBtn");
    nextBtn->setToolTip("Next week  Ctrl+\xe2\x86\x92");
    connect(nextBtn, &QPushButton::clicked, this, &MainWindow::onNextWeekClicked);

    QPushButton* todayBtn = new QPushButton("Today");
    todayBtn->setObjectName("weekNavBtn");
    todayBtn->setToolTip("Jump to the current week  Ctrl+T");
    connect(todayBtn, &QPushButton::clicked, this, &MainWindow::onTodayClicked);

    tb->addWidget(prevBtn);
    tb->addWidget(m_weekLabel);
    tb->addWidget(nextBtn);
    tb->addWidget(todayBtn);
    tb->addSeparator();

    // Action buttons
    QPushButton* generateBtn = new QPushButton("\xe2\x9a\xa1  Generate"); // ⚡
    generateBtn->setObjectName("primaryBtn");
    generateBtn->setToolTip("Auto-generate schedule for this week\n(locked assignments are preserved)  Ctrl+G");
    connect(generateBtn, &QPushButton::clicked, this, &MainWindow::onGenerateClicked);

    QPushButton* clearBtn = new QPushButton("\xe2\x9c\x95  Clear Unlocked"); // ✕
    clearBtn->setToolTip("Remove all unlocked assignments for this week");
    connect(clearBtn, &QPushButton::clicked, this, &MainWindow::onClearUnlockedClicked);

    QPushButton* exportBtn = new QPushButton("\xe2\x86\x93  Export CSV"); // ↓
    exportBtn->setToolTip("Export this week's schedule to CSV  Ctrl+E");
    connect(exportBtn, &QPushButton::clicked, this, &MainWindow::onExportCsvClicked);

    QPushButton* printBtn = new QPushButton("\xf0\x9f\x96\xa8  Print"); // 🖨
    printBtn->setToolTip("Print this week's schedule");
    connect(printBtn, &QPushButton::clicked, this, &MainWindow::onPrintClicked);

    tb->addWidget(generateBtn);
    tb->addWidget(clearBtn);
    tb->addSeparator();
    tb->addWidget(exportBtn);
    tb->addWidget(printBtn);

    // ── Keyboard shortcuts ────────────────────────────────────────────────────
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_G),     this, this, &MainWindow::onGenerateClicked);
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Left),  this, this, &MainWindow::onPrevWeekClicked);
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Right), this, this, &MainWindow::onNextWeekClicked);
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_T),     this, this, &MainWindow::onTodayClicked);
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_E),     this, this, &MainWindow::onExportCsvClicked);
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_P),     this, this, &MainWindow::onPrintClicked);
}

// ── Pages ─────────────────────────────────────────────────────────────────────

void MainWindow::buildPages() {
    // Page 0: Roles
    m_stack->addWidget(new RolesPage);

    // Page 1: Employees
    m_stack->addWidget(new EmployeesPage);

    // Page 2: Shifts
    m_stack->addWidget(new ShiftsPage);

    // Page 3: Schedule
    m_schedulePage = new SchedulePage;
    m_schedulePage->loadWeek(m_currentWeekStart);
    m_stack->addWidget(m_schedulePage);

    // Page 4: Reports
    m_reportsPage = new ReportsPage;
    m_stack->addWidget(m_reportsPage);

    // Page 5: Settings
    m_stack->addWidget(new SettingsPage);
}

// ── Navigation ────────────────────────────────────────────────────────────────

void MainWindow::navigateTo(Page page) {
    const int idx = static_cast<int>(page);
    m_stack->setCurrentIndex(idx);

    for (int i = 0; i < m_navBtns.size(); ++i)
        m_navBtns[i]->setChecked(i == idx);

    // Refresh data-driven pages on navigation
    if (page == Page::Schedule)
        m_schedulePage->loadWeek(m_currentWeekStart);
    if (page == Page::Reports)
        m_reportsPage->loadWeek(m_currentWeekStart);
}

// ── Week navigation ───────────────────────────────────────────────────────────

void MainWindow::onPrevWeekClicked() {
    QDate d = QDate::fromString(m_currentWeekStart, Qt::ISODate).addDays(-7);
    m_currentWeekStart = d.toString(Qt::ISODate);
    AppSettings::instance().setLastWeekStart(m_currentWeekStart);
    updateWeekLabel();
    m_schedulePage->loadWeek(m_currentWeekStart);
    m_reportsPage->loadWeek(m_currentWeekStart);
}

void MainWindow::onNextWeekClicked() {
    QDate d = QDate::fromString(m_currentWeekStart, Qt::ISODate).addDays(7);
    m_currentWeekStart = d.toString(Qt::ISODate);
    AppSettings::instance().setLastWeekStart(m_currentWeekStart);
    updateWeekLabel();
    m_schedulePage->loadWeek(m_currentWeekStart);
    m_reportsPage->loadWeek(m_currentWeekStart);
}

void MainWindow::onPrintClicked()
{
    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog dialog(&printer, this);
    dialog.setWindowTitle("Print Schedule");
    if (dialog.exec() != QDialog::Accepted) return;

    ScheduleRepository      schedRepo;
    ShiftTemplateRepository shiftRepo;
    EmployeeRepository      empRepo;
    RoleRepository          roleRepo;

    const Schedule schedule = schedRepo.getOrCreate(m_currentWeekStart);

    QHash<int, ShiftTemplate> shiftMap;
    for (const ShiftTemplate& st : shiftRepo.getAll())
        shiftMap.insert(st.id, st);

    QHash<int, QString> empNames;
    for (const Employee& e : empRepo.getAll())
        empNames.insert(e.id, e.name);

    QHash<int, QString> roleNames;
    for (const Role& r : roleRepo.getAll())
        roleNames.insert(r.id, r.name);

    QHash<QPair<int,int>, Assignment> assignMap;
    for (const Assignment& a : schedRepo.getAssignments(schedule.id))
        assignMap.insert(QPair<int,int>(a.shiftTemplateId, a.slotIndex), a);

    const bool use24h = AppSettings::instance().use24HourFormat();
    const QDate weekStart = QDate::fromString(m_currentWeekStart, Qt::ISODate);

    QString html;
    html += "<html><head><style>"
            "body{font-family:Arial,sans-serif;font-size:10pt;}"
            "h2{color:#333;margin-bottom:8px;}"
            "table{border-collapse:collapse;width:100%;}"
            "th{background:#4a4a8a;color:white;padding:6px;text-align:left;}"
            "td{border:1px solid #ccc;padding:4px;}"
            ".day{background:#dde;font-weight:bold;}"
            ".unassigned{color:#999;font-style:italic;}"
            "</style></head><body>";
    html += QString("<h2>Schedule &mdash; Week of %1 &ndash; %2</h2>")
                .arg(weekStart.toString("d MMM"))
                .arg(weekStart.addDays(6).toString("d MMM yyyy"));
    html += "<table><tr><th>Day</th><th>Shift</th><th>Time</th>"
            "<th>Role</th><th>Slot</th><th>Employee</th></tr>";

    int lastDay = -1;
    for (const ShiftTemplate& st : shiftRepo.getAll()) {
        if (st.dayOfWeek != lastDay) {
            lastDay = st.dayOfWeek;
            html += QString("<tr><td colspan='6' class='day'>%1</td></tr>")
                        .arg(TimeUtils::dayOfWeekName(st.dayOfWeek));
        }
        const QString timeStr =
            TimeUtils::minutesToTimeString(st.startMinute, use24h) + " &ndash; " +
            TimeUtils::minutesToTimeString(st.endMinute,   use24h);
        const QString role = roleNames.value(st.roleId, QString::number(st.roleId));

        for (int slot = 0; slot < st.staffRequired; ++slot) {
            const auto it = assignMap.constFind(QPair<int,int>(st.id, slot));
            const QString emp = (it != assignMap.constEnd())
                ? empNames.value(it->employeeId, QString("(emp %1)").arg(it->employeeId))
                : QString("<span class='unassigned'>&mdash;</span>");

            html += QString("<tr><td></td><td>%1</td><td>%2</td>"
                            "<td>%3</td><td>%4</td><td>%5</td></tr>")
                        .arg(st.name.toHtmlEscaped(), timeStr,
                             role.toHtmlEscaped(),
                             QString::number(slot + 1), emp);
        }
    }

    html += "</table></body></html>";

    QTextDocument doc;
    doc.setHtml(html);
    doc.print(&printer);

    statusBar()->showMessage("Schedule sent to printer.", 3000);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    AppSettings::instance().setLastWeekStart(m_currentWeekStart);
    QMainWindow::closeEvent(event);
}

void MainWindow::updateWeekLabel() {
    QDate weekStart = QDate::fromString(m_currentWeekStart, Qt::ISODate);
    QDate weekEnd   = weekStart.addDays(6);
    m_weekLabel->setText(
        QString("Week of %1 \xe2\x80\x93 %2")  // "–" en-dash
            .arg(weekStart.toString("d MMM"))
            .arg(weekEnd.toString("d MMM yyyy"))
    );
}

QString MainWindow::currentWeekStart() const {
    return m_currentWeekStart;
}

// ── Toolbar slot stubs ────────────────────────────────────────────────────────
// These are wired to real implementations in Phase 7.

void MainWindow::onGenerateClicked() {
    const int filled = Scheduler::generateWeek(m_currentWeekStart);
    m_schedulePage->loadWeek(m_currentWeekStart);
    m_reportsPage->loadWeek(m_currentWeekStart);
    onScheduleSaved();
    statusBar()->showMessage(
        QString("Schedule generated: %1 slot(s) filled.").arg(filled), 4000);
}

void MainWindow::onClearUnlockedClicked() {
    ScheduleRepository schedRepo;
    const Schedule schedule = schedRepo.getOrCreate(m_currentWeekStart);
    schedRepo.clearUnlocked(schedule.id);
    m_schedulePage->loadWeek(m_currentWeekStart);
    m_reportsPage->loadWeek(m_currentWeekStart);
    onScheduleSaved();
    statusBar()->showMessage("Unlocked assignments cleared.", 3000);
}

void MainWindow::onTodayClicked()
{
    QDate today = QDate::currentDate();
    const int daysToMon = (today.dayOfWeek() - 1 + 7) % 7;
    const QString weekStart = today.addDays(-daysToMon).toString(Qt::ISODate);
    if (weekStart == m_currentWeekStart) return;
    m_currentWeekStart = weekStart;
    AppSettings::instance().setLastWeekStart(m_currentWeekStart);
    updateWeekLabel();
    m_schedulePage->loadWeek(m_currentWeekStart);
    m_reportsPage->loadWeek(m_currentWeekStart);
}

void MainWindow::onScheduleSaved()
{
    m_saveIndicator->setStyleSheet("color: #50c878; font-weight: bold;");
    QTimer::singleShot(2500, this, [this]() {
        m_saveIndicator->setStyleSheet(QString());
    });
}

void MainWindow::onAboutClicked()
{
    QMessageBox::about(this, "About ShiftWise",
        QString("<b>ShiftWise v%1</b><br><br>"
                "Staff scheduling made simple.<br><br>"
                "A Qt6 desktop application for managing<br>"
                "employee shifts, roles, and weekly schedules.<br><br>"
                "<b>Author:</b> Antony Morrison<br>"
                "<b>Company:</b> Walking Fish Software<br><br>"
                "&copy; 2025 Walking Fish Software. All rights reserved.")
            .arg(SHIFTWISE_VERSION));
}

void MainWindow::onExportCsvClicked()
{
    const QString path = QFileDialog::getSaveFileName(
        this,
        "Export Schedule as CSV",
        QString("schedule_%1.csv").arg(m_currentWeekStart),
        "CSV files (*.csv);;All files (*)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Export Failed",
            QString("Could not write to:\n%1").arg(path));
        return;
    }

    ScheduleRepository      schedRepo;
    ShiftTemplateRepository shiftRepo;
    EmployeeRepository      empRepo;
    RoleRepository          roleRepo;

    const Schedule schedule = schedRepo.getOrCreate(m_currentWeekStart);

    // Build lookup maps
    QHash<int, ShiftTemplate> shiftMap;
    for (const ShiftTemplate& st : shiftRepo.getAll())
        shiftMap.insert(st.id, st);

    QHash<int, QString> empNames;
    for (const Employee& e : empRepo.getAll())
        empNames.insert(e.id, e.name);

    QHash<int, QString> roleNames;
    for (const Role& r : roleRepo.getAll())
        roleNames.insert(r.id, r.name);

    // Build assignment map: (shiftTemplateId, slotIndex) -> Assignment
    QHash<QPair<int,int>, Assignment> assignMap;
    for (const Assignment& a : schedRepo.getAssignments(schedule.id))
        assignMap.insert(QPair<int,int>(a.shiftTemplateId, a.slotIndex), a);

    const bool use24h = AppSettings::instance().use24HourFormat();

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    // Header
    out << "Day,Shift,Start,End,Role,Slot,Employee,Locked\n";

    for (const ShiftTemplate& st : shiftRepo.getAll()) {
        const QString day     = TimeUtils::dayOfWeekName(st.dayOfWeek);
        const QString start   = TimeUtils::minutesToTimeString(st.startMinute, use24h);
        const QString end     = TimeUtils::minutesToTimeString(st.endMinute,   use24h);
        const QString role    = roleNames.value(st.roleId, QString::number(st.roleId));

        for (int slot = 0; slot < st.staffRequired; ++slot) {
            const auto it = assignMap.constFind(QPair<int,int>(st.id, slot));
            QString empName;
            QString locked;
            if (it != assignMap.constEnd()) {
                empName = empNames.value(it->employeeId,
                              QString("(emp %1)").arg(it->employeeId));
                locked  = it->isLocked ? "Yes" : "No";
            }

            // Helper lambda: quote a field if it contains comma, quote or newline
            auto q = [](const QString& s) -> QString {
                if (s.contains(',') || s.contains('"') || s.contains('\n')) {
                    QString esc = s;
                    esc.replace('"', "\"\"");
                    return '"' + esc + '"';
                }
                return s;
            };

            out << q(day)     << ','
                << q(st.name) << ','
                << q(start)   << ','
                << q(end)     << ','
                << q(role)    << ','
                << (slot + 1) << ','
                << q(empName) << ','
                << q(locked)  << '\n';
        }
    }

    file.close();
    statusBar()->showMessage(
        QString("Exported to %1").arg(QFileInfo(file).fileName()), 4000);
}
