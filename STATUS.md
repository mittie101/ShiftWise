# ShiftWise — Project Status
**Last updated:** 2026-03-02
**Version:** 1.0.0
**Stack:** Qt 6.10.2 · C++17 · MinGW · CMake + Ninja · SQLite

---

## What it is
Desktop staff scheduling app for Windows. Manage roles, employees, shift templates, and weekly schedules. Features auto-scheduling, locking, CSV export, and printing.

---

## What's working (as of today)
- **All 6 pages fully functional:** Roles, Employees, Shifts, Schedule, Reports, Settings
- **Schedule page:** list view + calendar view, assign employees to slots, lock/unlock assignments
- **Auto-scheduler:** generates a week's assignments respecting role, availability, hours cap, and overlap rules
- **Reports page:** fill-rate summary, per-employee hours, unfilled slots, CSV export
- **Export/Print:** CSV export and print dialog from toolbar
- **Settings:** DB path, overtime threshold, 24h time, dark/light theme, work week start day
- **Installer:** `installer/output/ShiftWise-1.0.0-Setup.exe` — builds via `deploy.bat`
- **Tests:** Catch2 suite in `tests/` covering repositories and domain logic

---

## Bugs fixed today (2026-03-02)
1. **Logger mutex deadlock** (`src/app/Logger.cpp`) — `Logger::init()` held `QMutex` then called `write()` which re-locked it. Changed to `QRecursiveMutex`. This caused a silent hang on every launch (process visible in Task Manager, no window).
2. **SQL migration splitter** (`migrations/migration_001.sql`) — `MigrationRunner` splits on `;` naively; two inline `--` comments contained `;` which broke `CREATE TABLE availability` and `CREATE TABLE schedules`. Removed semicolons from those comments. Also removed premature `INSERT INTO schema_version` from the SQL (code records the version after all tables succeed).

---

## File map
```
src/
  app/          Logger, AppSettings, MainWindow, main
  infra/        DatabaseManager, MigrationRunner
  domain/
    models/     Role, Employee, Availability, ShiftTemplate, Schedule, Assignment
    repositories/  5 repos (Role, Employee, Availability, ShiftTemplate, Schedule)
    Scheduler.cpp  auto-generate algorithm
    TimeUtils.h    header-only time helpers
  ui/
    pages/      RolesPage, EmployeesPage, ShiftsPage, SchedulePage, ReportsPage, SettingsPage
    dialogs/    RoleDialog, EmployeeDialog, ShiftTemplateDialog
migrations/
  migration_001.sql   full DB schema (7 tables + indexes)
resources/
  resources.qrc
  styles/dark.qss, light.qss
  icons/
tests/
  test_main.cpp   Catch2 tests
installer/
  shiftwise.iss   Inno Setup script
deploy.bat        full build → deploy → installer pipeline
```

---

## Build commands
```bash
# Release build + installer
deploy.bat

# Quick rebuild only (after source changes)
PATH="/c/Qt/Tools/mingw1310_64/bin:$PATH"
/c/Qt/Tools/CMake_64/bin/cmake.exe --build build/release --target ShiftWise

# Copy to deploy folder
cp build/release/ShiftWise.exe deploy/
```

---

## Next / potential work
- Nothing specifically planned — app is feature-complete for v1.0
- Could add: drag-and-drop schedule editing, recurring shift templates, employee notes, multi-week view
