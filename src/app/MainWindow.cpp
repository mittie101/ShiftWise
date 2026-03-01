#include "MainWindow.h"
#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QToolBar>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QDate>
#include <QStatusBar>

// ── Placeholder page widget ───────────────────────────────────────────────────
static QWidget* makePlaceholderPage(const QString& title, const QString& subtitle) {
    QWidget* page = new QWidget;
    QVBoxLayout* vl = new QVBoxLayout(page);
    vl->setAlignment(Qt::AlignCenter);
    vl->setSpacing(8);

    QLabel* titleLbl = new QLabel(title);
    titleLbl->setObjectName("sectionTitle");
    titleLbl->setAlignment(Qt::AlignCenter);
    QFont f = titleLbl->font();
    f.setPointSize(20);
    f.setBold(true);
    titleLbl->setFont(f);

    QLabel* subLbl = new QLabel(subtitle);
    subLbl->setObjectName("emptyState");
    subLbl->setAlignment(Qt::AlignCenter);

    vl->addWidget(titleLbl);
    vl->addWidget(subLbl);
    return page;
}

// ─────────────────────────────────────────────────────────────────────────────

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("ShiftWise");
    setMinimumSize(1100, 680);
    resize(1280, 780);

    // Compute initial week start (most recent Monday)
    QDate today = QDate::currentDate();
    int daysToMon = (today.dayOfWeek() - 1 + 7) % 7; // dayOfWeek: 1=Mon..7=Sun
    m_currentWeekStart = today.addDays(-daysToMon).toString(Qt::ISODate);

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
    QPushButton* prevBtn = new QPushButton("◀");
    prevBtn->setObjectName("weekNavBtn");
    prevBtn->setToolTip("Previous week");
    connect(prevBtn, &QPushButton::clicked, this, &MainWindow::onPrevWeekClicked);

    m_weekLabel = new QLabel;
    m_weekLabel->setMinimumWidth(180);
    m_weekLabel->setAlignment(Qt::AlignCenter);
    updateWeekLabel();

    QPushButton* nextBtn = new QPushButton("▶");
    nextBtn->setObjectName("weekNavBtn");
    nextBtn->setToolTip("Next week");
    connect(nextBtn, &QPushButton::clicked, this, &MainWindow::onNextWeekClicked);

    tb->addWidget(prevBtn);
    tb->addWidget(m_weekLabel);
    tb->addWidget(nextBtn);
    tb->addSeparator();

    // Action buttons
    QPushButton* generateBtn = new QPushButton("⚡  Generate");
    generateBtn->setObjectName("primaryBtn");
    generateBtn->setToolTip("Auto-generate schedule for this week\n(locked assignments are preserved)");
    connect(generateBtn, &QPushButton::clicked, this, &MainWindow::onGenerateClicked);

    QPushButton* clearBtn = new QPushButton("✕  Clear Unlocked");
    clearBtn->setToolTip("Remove all unlocked assignments for this week");
    connect(clearBtn, &QPushButton::clicked, this, &MainWindow::onClearUnlockedClicked);

    QPushButton* exportBtn = new QPushButton("↓  Export CSV");
    exportBtn->setToolTip("Export this week's schedule to CSV");
    connect(exportBtn, &QPushButton::clicked, this, &MainWindow::onExportCsvClicked);

    tb->addWidget(generateBtn);
    tb->addWidget(clearBtn);
    tb->addSeparator();
    tb->addWidget(exportBtn);
}

// ── Pages ─────────────────────────────────────────────────────────────────────

void MainWindow::buildPages() {
    // Phase 1: placeholder pages — replaced in Phase 5 with real views
    m_stack->addWidget(makePlaceholderPage("Employees",
        "Manage your employees, roles, and availability windows."));

    m_stack->addWidget(makePlaceholderPage("Shifts",
        "Define shift templates for each day of the week."));

    m_stack->addWidget(makePlaceholderPage("Schedule",
        "Generate and manage your weekly schedule here."));

    m_stack->addWidget(makePlaceholderPage("Reports",
        "View hours, overtime risks, and fairness scores."));

    m_stack->addWidget(makePlaceholderPage("Settings",
        "Configure overtime thresholds, display preferences, and database location."));
}

// ── Navigation ────────────────────────────────────────────────────────────────

void MainWindow::navigateTo(Page page) {
    const int idx = static_cast<int>(page);
    m_stack->setCurrentIndex(idx);

    for (int i = 0; i < m_navBtns.size(); ++i) {
        m_navBtns[i]->setChecked(i == idx);
    }
}

// ── Week navigation ───────────────────────────────────────────────────────────

void MainWindow::onPrevWeekClicked() {
    QDate d = QDate::fromString(m_currentWeekStart, Qt::ISODate).addDays(-7);
    m_currentWeekStart = d.toString(Qt::ISODate);
    updateWeekLabel();
}

void MainWindow::onNextWeekClicked() {
    QDate d = QDate::fromString(m_currentWeekStart, Qt::ISODate).addDays(7);
    m_currentWeekStart = d.toString(Qt::ISODate);
    updateWeekLabel();
}

void MainWindow::updateWeekLabel() {
    QDate weekStart = QDate::fromString(m_currentWeekStart, Qt::ISODate);
    QDate weekEnd   = weekStart.addDays(6);
    m_weekLabel->setText(
        QString("Week of %1 – %2")
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
    statusBar()->showMessage("Generate schedule — coming in Phase 7.", 3000);
}

void MainWindow::onClearUnlockedClicked() {
    statusBar()->showMessage("Clear unlocked — coming in Phase 7.", 3000);
}

void MainWindow::onExportCsvClicked() {
    statusBar()->showMessage("Export CSV — coming in Phase 8.", 3000);
}
