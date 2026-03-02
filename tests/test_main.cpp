#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>
#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>
#include <QCoreApplication>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>

#include "domain/repositories/RoleRepository.h"
#include "domain/repositories/EmployeeRepository.h"
#include "domain/repositories/AvailabilityRepository.h"
#include "domain/models/Availability.h"
#include "domain/repositories/ShiftTemplateRepository.h"
#include "domain/repositories/ScheduleRepository.h"
#include "domain/Scheduler.h"

// Ensure a QCoreApplication exists for the entire test session.
// Catch2WithMain provides main(); we hook in via a session listener.
namespace {

class QtAppListener : public Catch::EventListenerBase {
public:
    using Catch::EventListenerBase::EventListenerBase;

    void testRunStarting(Catch::TestRunInfo const&) override
    {
        // Create if not already present (only once)
        if (!QCoreApplication::instance()) {
            static int   argc = 0;
            static char* argv[] = { nullptr };
            m_app = std::make_unique<QCoreApplication>(argc, argv);
        }
    }

private:
    std::unique_ptr<QCoreApplication> m_app;
};

CATCH_REGISTER_LISTENER(QtAppListener)

} // namespace

// ── Phase 1 scaffold ─────────────────────────────────────────────────────────

TEST_CASE("Catch2 is wired correctly", "[scaffold]") {
    REQUIRE(1 + 1 == 2);
}

// ── Phase 4: RoleRepository smoke test ───────────────────────────────────────

namespace {

void setupRoleTable(const QString& connName)
{
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
    db.setDatabaseName(QStringLiteral(":memory:"));
    REQUIRE(db.open());

    QSqlQuery q(db);
    REQUIRE(q.exec(QStringLiteral(
        "CREATE TABLE roles ("
        "  id        INTEGER PRIMARY KEY,"
        "  name      TEXT    NOT NULL UNIQUE,"
        "  colourHex TEXT    NOT NULL DEFAULT '#e94560'"
        ")")));
}

void teardownConn(const QString& connName)
{
    QSqlDatabase::database(connName).close();
    QSqlDatabase::removeDatabase(connName);
}

} // namespace

TEST_CASE("RoleRepository: insert, getById, update, remove", "[domain][role]")
{
    const QString kConn = QStringLiteral("test_role_conn");
    setupRoleTable(kConn);

    RoleRepository repo(kConn);

    // Insert
    Role r;
    r.name      = QStringLiteral("Barista");
    r.colourHex = QStringLiteral("#aabbcc");

    int newId = repo.insert(r);
    REQUIRE(newId > 0);

    // getById — fields match
    auto fetched = repo.getById(newId);
    REQUIRE(fetched.has_value());
    CHECK(fetched->id        == newId);
    CHECK(fetched->name      == QStringLiteral("Barista"));
    CHECK(fetched->colourHex == QStringLiteral("#aabbcc"));

    // update
    fetched->name      = QStringLiteral("Manager");
    fetched->colourHex = QStringLiteral("#112233");
    REQUIRE(repo.update(*fetched));

    auto updated = repo.getById(newId);
    REQUIRE(updated.has_value());
    CHECK(updated->name      == QStringLiteral("Manager"));
    CHECK(updated->colourHex == QStringLiteral("#112233"));

    // remove
    REQUIRE(repo.remove(newId));
    CHECK_FALSE(repo.getById(newId).has_value());

    // remove non-existent returns false
    CHECK_FALSE(repo.remove(newId));

    teardownConn(kConn);
}

// ── Shared helpers ────────────────────────────────────────────────────────────

namespace {

// Creates an in-memory SQLite DB with the full ShiftWise schema.
void setupFullSchema(const QString& connName)
{
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
    db.setDatabaseName(QStringLiteral(":memory:"));
    REQUIRE(db.open());

    QSqlQuery q(db);
    q.exec("PRAGMA foreign_keys = ON");
    REQUIRE(q.exec("CREATE TABLE roles (id INTEGER PRIMARY KEY, name TEXT NOT NULL UNIQUE, colourHex TEXT NOT NULL DEFAULT '#e94560')"));
    REQUIRE(q.exec("CREATE TABLE employees (id INTEGER PRIMARY KEY, name TEXT NOT NULL, maxWeeklyMinutes INTEGER NOT NULL DEFAULT 2400, priority INTEGER NOT NULL DEFAULT 0)"));
    REQUIRE(q.exec("CREATE TABLE employee_roles (employeeId INTEGER NOT NULL, roleId INTEGER NOT NULL, PRIMARY KEY (employeeId, roleId))"));
    REQUIRE(q.exec("CREATE TABLE availability (id INTEGER PRIMARY KEY, employeeId INTEGER NOT NULL, dayOfWeek INTEGER NOT NULL, startMinute INTEGER NOT NULL, endMinute INTEGER NOT NULL)"));
    REQUIRE(q.exec("CREATE TABLE shift_templates (id INTEGER PRIMARY KEY, name TEXT, dayOfWeek INTEGER NOT NULL, startMinute INTEGER NOT NULL, endMinute INTEGER NOT NULL, roleId INTEGER NOT NULL, staffRequired INTEGER NOT NULL DEFAULT 1)"));
    REQUIRE(q.exec("CREATE TABLE schedules (id INTEGER PRIMARY KEY, weekStart TEXT NOT NULL, createdAt TEXT, UNIQUE (weekStart))"));
    REQUIRE(q.exec("CREATE TABLE assignments (id INTEGER PRIMARY KEY, scheduleId INTEGER NOT NULL, shiftTemplateId INTEGER NOT NULL, slotIndex INTEGER NOT NULL DEFAULT 0, employeeId INTEGER NOT NULL, isLocked INTEGER NOT NULL DEFAULT 0, UNIQUE (scheduleId, shiftTemplateId, slotIndex))"));
}

// "2025-01-06" is a Monday — used as the test week.
constexpr auto kTestWeek = "2025-01-06";

} // namespace

// ── EmployeeRepository CRUD ───────────────────────────────────────────────────

TEST_CASE("EmployeeRepository: insert, getById, update, remove", "[domain][employee]")
{
    const QString kConn = "test_emp_conn";
    setupFullSchema(kConn);

    EmployeeRepository repo(kConn);

    Employee e;
    e.name             = "Alice";
    e.maxWeeklyMinutes = 2400;
    e.priority         = 3;

    const int id = repo.insert(e);
    REQUIRE(id > 0);

    auto fetched = repo.getById(id);
    REQUIRE(fetched.has_value());
    CHECK(fetched->name             == "Alice");
    CHECK(fetched->maxWeeklyMinutes == 2400);
    CHECK(fetched->priority         == 3);

    fetched->name             = "Alicia";
    fetched->maxWeeklyMinutes = 1800;
    REQUIRE(repo.update(*fetched));

    auto updated = repo.getById(id);
    REQUIRE(updated.has_value());
    CHECK(updated->name             == "Alicia");
    CHECK(updated->maxWeeklyMinutes == 1800);

    REQUIRE(repo.remove(id));
    CHECK_FALSE(repo.getById(id).has_value());

    teardownConn(kConn);
}

// ── ScheduleRepository: getOrCreate is idempotent ────────────────────────────

TEST_CASE("ScheduleRepository: getOrCreate returns same id on second call", "[domain][schedule]")
{
    const QString kConn = "test_sched_conn";
    setupFullSchema(kConn);

    ScheduleRepository repo(kConn);

    const Schedule s1 = repo.getOrCreate(kTestWeek);
    const Schedule s2 = repo.getOrCreate(kTestWeek);

    REQUIRE(s1.id > 0);
    CHECK(s1.id == s2.id);
    CHECK(s1.weekStart == kTestWeek);

    teardownConn(kConn);
}

// ── Scheduler: basic slot is filled ──────────────────────────────────────────

TEST_CASE("Scheduler: fills slot when employee has matching role and availability", "[scheduler][basic]")
{
    const QString kConn = "test_sched_basic";
    setupFullSchema(kConn);

    RoleRepository          roleRepo(kConn);
    EmployeeRepository      empRepo(kConn);
    AvailabilityRepository  availRepo(kConn);
    ShiftTemplateRepository shiftRepo(kConn);
    ScheduleRepository      schedRepo(kConn);

    Role r;  r.name = "Barista"; r.colourHex = "#aaa";
    const int roleId = roleRepo.insert(r);

    Employee e;  e.name = "Alice";  e.maxWeeklyMinutes = 2400;
    const int empId = empRepo.insert(e);
    empRepo.setRolesForEmployee(empId, {roleId});

    Availability av;
    av.employeeId = empId;  av.dayOfWeek = 0;   // Monday
    av.startMinute = 540;   av.endMinute = 1020; // 9:00–17:00
    availRepo.insert(av);

    ShiftTemplate st;
    st.name = "Morning";  st.dayOfWeek = 0;
    st.startMinute = 540; st.endMinute = 1020;
    st.roleId = roleId;   st.staffRequired = 1;
    shiftRepo.insert(st);

    const int filled = Scheduler::generateWeek(kTestWeek, kConn);
    CHECK(filled == 1);

    const Schedule sched = schedRepo.getOrCreate(kTestWeek);
    const auto assignments = schedRepo.getAssignments(sched.id);
    REQUIRE(assignments.size() == 1);
    CHECK(assignments[0].employeeId == empId);

    teardownConn(kConn);
}

// ── Scheduler: role mismatch means no assignment ──────────────────────────────

TEST_CASE("Scheduler: skips employee who lacks required role", "[scheduler][role]")
{
    const QString kConn = "test_sched_role";
    setupFullSchema(kConn);

    RoleRepository          roleRepo(kConn);
    EmployeeRepository      empRepo(kConn);
    AvailabilityRepository  availRepo(kConn);
    ShiftTemplateRepository shiftRepo(kConn);
    ScheduleRepository      schedRepo(kConn);

    Role r1; r1.name = "Barista";  r1.colourHex = "#aaa";
    Role r2; r2.name = "Manager";  r2.colourHex = "#bbb";
    const int baristaId = roleRepo.insert(r1);
    const int managerId = roleRepo.insert(r2);

    Employee e; e.name = "Bob"; e.maxWeeklyMinutes = 2400;
    const int empId = empRepo.insert(e);
    empRepo.setRolesForEmployee(empId, {baristaId}); // only Barista

    Availability av;
    av.employeeId = empId; av.dayOfWeek = 0;
    av.startMinute = 0;    av.endMinute = 1440;
    availRepo.insert(av);

    ShiftTemplate st; // requires Manager
    st.name = "Manager Shift"; st.dayOfWeek = 0;
    st.startMinute = 540;      st.endMinute = 1020;
    st.roleId = managerId;     st.staffRequired = 1;
    shiftRepo.insert(st);

    const int filled = Scheduler::generateWeek(kTestWeek, kConn);
    CHECK(filled == 0);

    const Schedule sched = schedRepo.getOrCreate(kTestWeek);
    CHECK(schedRepo.getAssignments(sched.id).isEmpty());

    teardownConn(kConn);
}

// ── Scheduler: capacity limit respected ──────────────────────────────────────

TEST_CASE("Scheduler: skips employee who would exceed weekly capacity", "[scheduler][capacity]")
{
    const QString kConn = "test_sched_cap";
    setupFullSchema(kConn);

    RoleRepository          roleRepo(kConn);
    EmployeeRepository      empRepo(kConn);
    AvailabilityRepository  availRepo(kConn);
    ShiftTemplateRepository shiftRepo(kConn);
    ScheduleRepository      schedRepo(kConn);

    Role r; r.name = "Staff"; r.colourHex = "#aaa";
    const int roleId = roleRepo.insert(r);

    Employee e; e.name = "Carol"; e.maxWeeklyMinutes = 60; // only 1 hour allowed
    const int empId = empRepo.insert(e);
    empRepo.setRolesForEmployee(empId, {roleId});

    Availability av;
    av.employeeId = empId; av.dayOfWeek = 0;
    av.startMinute = 0;    av.endMinute = 1440;
    availRepo.insert(av);

    ShiftTemplate st; // 8-hour shift
    st.name = "Long Shift"; st.dayOfWeek = 0;
    st.startMinute = 540;   st.endMinute = 1020; // 480 min > 60
    st.roleId = roleId;     st.staffRequired = 1;
    shiftRepo.insert(st);

    const int filled = Scheduler::generateWeek(kTestWeek, kConn);
    CHECK(filled == 0);

    teardownConn(kConn);
}

// ── Scheduler: availability check ────────────────────────────────────────────

TEST_CASE("Scheduler: skips employee not available on shift day", "[scheduler][availability]")
{
    const QString kConn = "test_sched_avail";
    setupFullSchema(kConn);

    RoleRepository          roleRepo(kConn);
    EmployeeRepository      empRepo(kConn);
    AvailabilityRepository  availRepo(kConn);
    ShiftTemplateRepository shiftRepo(kConn);
    ScheduleRepository      schedRepo(kConn);

    Role r; r.name = "Staff"; r.colourHex = "#aaa";
    const int roleId = roleRepo.insert(r);

    Employee e; e.name = "Dave"; e.maxWeeklyMinutes = 2400;
    const int empId = empRepo.insert(e);
    empRepo.setRolesForEmployee(empId, {roleId});

    Availability av;
    av.employeeId = empId; av.dayOfWeek = 1; // Tuesday only
    av.startMinute = 540;  av.endMinute = 1020;
    availRepo.insert(av);

    ShiftTemplate st; // Monday shift
    st.name = "Mon Morning"; st.dayOfWeek = 0; // Monday
    st.startMinute = 540;    st.endMinute = 1020;
    st.roleId = roleId;      st.staffRequired = 1;
    shiftRepo.insert(st);

    const int filled = Scheduler::generateWeek(kTestWeek, kConn);
    CHECK(filled == 0);

    teardownConn(kConn);
}

// ── Scheduler: conflict detection — no double-booking ─────────────────────────

TEST_CASE("Scheduler: does not assign same employee to two overlapping shifts on the same day", "[scheduler][conflict]")
{
    const QString kConn = "test_sched_conflict";
    setupFullSchema(kConn);

    RoleRepository          roleRepo(kConn);
    EmployeeRepository      empRepo(kConn);
    AvailabilityRepository  availRepo(kConn);
    ShiftTemplateRepository shiftRepo(kConn);
    ScheduleRepository      schedRepo(kConn);

    Role r; r.name = "Staff"; r.colourHex = "#aaa";
    const int roleId = roleRepo.insert(r);

    // Single employee with ample capacity and all-day availability
    Employee e; e.name = "Eve"; e.maxWeeklyMinutes = 2400;
    const int empId = empRepo.insert(e);
    empRepo.setRolesForEmployee(empId, {roleId});

    Availability av;
    av.employeeId = empId; av.dayOfWeek = 0;
    av.startMinute = 0;    av.endMinute = 1440;
    availRepo.insert(av);

    // Two shifts that overlap: 9–13 and 11–15
    ShiftTemplate st1;
    st1.name = "Morning"; st1.dayOfWeek = 0;
    st1.startMinute = 540; st1.endMinute = 780; // 9:00–13:00
    st1.roleId = roleId;  st1.staffRequired = 1;
    shiftRepo.insert(st1);

    ShiftTemplate st2;
    st2.name = "Midday"; st2.dayOfWeek = 0;
    st2.startMinute = 660; st2.endMinute = 900; // 11:00–15:00
    st2.roleId = roleId;  st2.staffRequired = 1;
    shiftRepo.insert(st2);

    const int filled = Scheduler::generateWeek(kTestWeek, kConn);

    // Only one of the two overlapping slots should be filled
    CHECK(filled == 1);

    const Schedule sched = schedRepo.getOrCreate(kTestWeek);
    const auto assignments = schedRepo.getAssignments(sched.id);
    CHECK(assignments.size() == 1);
    // The assigned employee must be Eve
    CHECK(assignments[0].employeeId == empId);

    teardownConn(kConn);
}

// ── Scheduler: midnight-crossing shift ───────────────────────────────────────

TEST_CASE("Scheduler: assigns midnight-crossing shift to employee available at start time", "[scheduler][midnight]")
{
    const QString kConn = "test_sched_midnight";
    setupFullSchema(kConn);

    RoleRepository          roleRepo(kConn);
    EmployeeRepository      empRepo(kConn);
    AvailabilityRepository  availRepo(kConn);
    ShiftTemplateRepository shiftRepo(kConn);
    ScheduleRepository      schedRepo(kConn);

    Role r; r.name = "Night Staff"; r.colourHex = "#333";
    const int roleId = roleRepo.insert(r);

    Employee e; e.name = "Frank"; e.maxWeeklyMinutes = 2400;
    const int empId = empRepo.insert(e);
    empRepo.setRolesForEmployee(empId, {roleId});

    // Available Mon from 22:00 (1320) — covers shift start
    Availability av;
    av.employeeId = empId; av.dayOfWeek = 0;
    av.startMinute = 1320; av.endMinute = 1440; // 22:00–24:00
    availRepo.insert(av);

    // Midnight-crossing shift: Mon 22:00 (1320) to 02:00 (120)
    ShiftTemplate st;
    st.name = "Night"; st.dayOfWeek = 0;
    st.startMinute = 1320; st.endMinute = 120; // endMinute < startMinute → midnight crossing
    st.roleId = roleId;   st.staffRequired = 1;
    shiftRepo.insert(st);

    const int filled = Scheduler::generateWeek(kTestWeek, kConn);
    CHECK(filled == 1);

    const Schedule sched = schedRepo.getOrCreate(kTestWeek);
    const auto assignments = schedRepo.getAssignments(sched.id);
    REQUIRE(assignments.size() == 1);
    CHECK(assignments[0].employeeId == empId);

    teardownConn(kConn);
}

// ── Scheduler: multi-slot shift (staffRequired > 1) ──────────────────────────

TEST_CASE("Scheduler: fills all slots of a multi-staff shift with different employees", "[scheduler][multislot]")
{
    const QString kConn = "test_sched_multi";
    setupFullSchema(kConn);

    RoleRepository          roleRepo(kConn);
    EmployeeRepository      empRepo(kConn);
    AvailabilityRepository  availRepo(kConn);
    ShiftTemplateRepository shiftRepo(kConn);
    ScheduleRepository      schedRepo(kConn);

    Role r; r.name = "Staff"; r.colourHex = "#aaa";
    const int roleId = roleRepo.insert(r);

    // Two employees, both eligible
    for (const QString& name : {QStringLiteral("Grace"), QStringLiteral("Hank")}) {
        Employee e; e.name = name; e.maxWeeklyMinutes = 2400;
        const int empId = empRepo.insert(e);
        empRepo.setRolesForEmployee(empId, {roleId});

        Availability av;
        av.employeeId = empId; av.dayOfWeek = 0;
        av.startMinute = 540;  av.endMinute = 1020;
        availRepo.insert(av);
    }

    // Shift requires 2 staff
    ShiftTemplate st;
    st.name = "Double"; st.dayOfWeek = 0;
    st.startMinute = 540; st.endMinute = 1020;
    st.roleId = roleId;  st.staffRequired = 2;
    shiftRepo.insert(st);

    const int filled = Scheduler::generateWeek(kTestWeek, kConn);
    CHECK(filled == 2);

    const Schedule sched = schedRepo.getOrCreate(kTestWeek);
    const auto assignments = schedRepo.getAssignments(sched.id);
    REQUIRE(assignments.size() == 2);

    // Each slot has a different employee
    CHECK(assignments[0].employeeId != assignments[1].employeeId);

    teardownConn(kConn);
}

// ── Scheduler: locked assignments survive re-generate ────────────────────────

TEST_CASE("Scheduler: locked assignment is preserved on re-generate", "[scheduler][locked]")
{
    const QString kConn = "test_sched_locked";
    setupFullSchema(kConn);

    RoleRepository          roleRepo(kConn);
    EmployeeRepository      empRepo(kConn);
    AvailabilityRepository  availRepo(kConn);
    ShiftTemplateRepository shiftRepo(kConn);
    ScheduleRepository      schedRepo(kConn);

    Role r; r.name = "Staff"; r.colourHex = "#aaa";
    const int roleId = roleRepo.insert(r);

    // Two eligible employees
    int ids[2];
    for (int i = 0; i < 2; ++i) {
        Employee e; e.name = (i == 0) ? "Iris" : "Jack"; e.maxWeeklyMinutes = 2400;
        ids[i] = empRepo.insert(e);
        empRepo.setRolesForEmployee(ids[i], {roleId});

        Availability av;
        av.employeeId = ids[i]; av.dayOfWeek = 0;
        av.startMinute = 540;   av.endMinute = 1020;
        availRepo.insert(av);
    }

    ShiftTemplate st;
    st.name = "Morning"; st.dayOfWeek = 0;
    st.startMinute = 540; st.endMinute = 1020;
    st.roleId = roleId;  st.staffRequired = 1;
    const int stId = shiftRepo.insert(st);

    // First generate — one of the two employees gets assigned
    Scheduler::generateWeek(kTestWeek, kConn);

    const Schedule sched = schedRepo.getOrCreate(kTestWeek);
    auto first = schedRepo.getAssignments(sched.id);
    REQUIRE(first.size() == 1);

    // Lock the assignment
    Assignment locked = first[0];
    const int lockedEmpId = locked.employeeId;
    locked.isLocked = true;
    schedRepo.upsertAssignment(locked);

    // Re-generate (clears unlocked, preserves locked)
    schedRepo.clearUnlocked(sched.id);
    Scheduler::generateWeek(kTestWeek, kConn);

    const auto after = schedRepo.getAssignments(sched.id);
    bool lockedPreserved = false;
    for (const Assignment& a : after)
        if (a.shiftTemplateId == stId && a.employeeId == lockedEmpId && a.isLocked)
            lockedPreserved = true;

    CHECK(lockedPreserved);

    teardownConn(kConn);
}

// ── Scheduler: locked assignment prevents cross-shift double-booking ──────────

TEST_CASE("Scheduler: employee locked on overlapping shift is not double-booked", "[scheduler][locked][conflict]")
{
    const QString kConn = "test_sched_locked_conflict";
    setupFullSchema(kConn);

    RoleRepository          roleRepo(kConn);
    EmployeeRepository      empRepo(kConn);
    AvailabilityRepository  availRepo(kConn);
    ShiftTemplateRepository shiftRepo(kConn);
    ScheduleRepository      schedRepo(kConn);

    Role r; r.name = "Staff"; r.colourHex = "#aaa";
    const int roleId = roleRepo.insert(r);

    // Single employee
    Employee e; e.name = "Kay"; e.maxWeeklyMinutes = 2400;
    const int empId = empRepo.insert(e);
    empRepo.setRolesForEmployee(empId, {roleId});

    Availability av;
    av.employeeId = empId; av.dayOfWeek = 0;
    av.startMinute = 0;    av.endMinute = 1440;
    availRepo.insert(av);

    // Shift 1: Mon 9–13 — will be locked
    ShiftTemplate st1;
    st1.name = "Morning"; st1.dayOfWeek = 0;
    st1.startMinute = 540; st1.endMinute = 780;  // 9:00–13:00
    st1.roleId = roleId;  st1.staffRequired = 1;
    const int stId1 = shiftRepo.insert(st1);

    // Shift 2: Mon 11–15 — overlaps with shift 1
    ShiftTemplate st2;
    st2.name = "Midday"; st2.dayOfWeek = 0;
    st2.startMinute = 660; st2.endMinute = 900;  // 11:00–15:00
    st2.roleId = roleId;  st2.staffRequired = 1;
    shiftRepo.insert(st2);

    // Manually insert a locked assignment for shift 1
    const Schedule sched = schedRepo.getOrCreate(kTestWeek);
    Assignment a;
    a.scheduleId = sched.id; a.shiftTemplateId = stId1;
    a.slotIndex = 0; a.employeeId = empId; a.isLocked = true;
    schedRepo.upsertAssignment(a);

    // Generate — shift 2 overlaps, so Kay should NOT be double-booked
    Scheduler::generateWeek(kTestWeek, kConn);

    const auto assignments = schedRepo.getAssignments(sched.id);
    // Only the locked assignment for shift 1 should exist
    int countForKay = 0;
    for (const Assignment& asgn : assignments)
        if (asgn.employeeId == empId) ++countForKay;

    CHECK(countForKay == 1);

    teardownConn(kConn);
}

// ── Scheduler: fairness — employee with fewer hours gets priority ─────────────

TEST_CASE("Scheduler: assigns to employee with fewer scheduled hours first", "[scheduler][fairness]")
{
    const QString kConn = "test_sched_fair";
    setupFullSchema(kConn);

    RoleRepository          roleRepo(kConn);
    EmployeeRepository      empRepo(kConn);
    AvailabilityRepository  availRepo(kConn);
    ShiftTemplateRepository shiftRepo(kConn);
    ScheduleRepository      schedRepo(kConn);

    Role r; r.name = "Staff"; r.colourHex = "#aaa";
    const int roleId = roleRepo.insert(r);

    // Two employees with equal priority
    Employee eA; eA.name = "Aaron"; eA.maxWeeklyMinutes = 2400; eA.priority = 0;
    Employee eB; eB.name = "Betty"; eB.maxWeeklyMinutes = 2400; eB.priority = 0;
    const int idA = empRepo.insert(eA);
    const int idB = empRepo.insert(eB);
    empRepo.setRolesForEmployee(idA, {roleId});
    empRepo.setRolesForEmployee(idB, {roleId});

    for (int emp : {idA, idB}) {
        Availability av;
        av.employeeId = emp; av.dayOfWeek = 0;
        av.startMinute = 0;  av.endMinute = 1440;
        availRepo.insert(av);
    }

    // Shift 1 (Mon 9–13): both eligible — scheduler picks Aaron first (alphabetical/id order)
    ShiftTemplate st1;
    st1.name = "Shift A"; st1.dayOfWeek = 0;
    st1.startMinute = 540; st1.endMinute = 780; // 9:00–13:00 (240 min)
    st1.roleId = roleId;  st1.staffRequired = 1;
    const int stId1 = shiftRepo.insert(st1);

    // Shift 2 (Mon 14–18): after shift 1, Aaron has 240 min, Betty has 0 min → Betty gets it
    ShiftTemplate st2;
    st2.name = "Shift B"; st2.dayOfWeek = 0;
    st2.startMinute = 840; st2.endMinute = 1080; // 14:00–18:00 (240 min)
    st2.roleId = roleId;  st2.staffRequired = 1;
    const int stId2 = shiftRepo.insert(st2);

    const int filled = Scheduler::generateWeek(kTestWeek, kConn);
    CHECK(filled == 2);

    const Schedule sched = schedRepo.getOrCreate(kTestWeek);
    const auto assignments = schedRepo.getAssignments(sched.id);
    REQUIRE(assignments.size() == 2);

    // Find which employee got which shift
    int empForSt1 = -1, empForSt2 = -1;
    for (const Assignment& a : assignments) {
        if (a.shiftTemplateId == stId1) empForSt1 = a.employeeId;
        if (a.shiftTemplateId == stId2) empForSt2 = a.employeeId;
    }

    // The two shifts go to different employees
    CHECK(empForSt1 != -1);
    CHECK(empForSt2 != -1);
    CHECK(empForSt1 != empForSt2);

    teardownConn(kConn);
}

// ── AvailabilityRepository CRUD ───────────────────────────────────────────────

TEST_CASE("AvailabilityRepository: insert, getById, getForEmployee, update, remove", "[domain][availability]")
{
    const QString kConn = "test_avail_crud";
    setupFullSchema(kConn);

    EmployeeRepository     empRepo(kConn);
    AvailabilityRepository availRepo(kConn);

    // Need an employee to satisfy the domain model
    Employee e; e.name = "Lena"; e.maxWeeklyMinutes = 2400;
    const int empId = empRepo.insert(e);
    REQUIRE(empId > 0);

    // insert
    Availability av;
    av.employeeId  = empId;
    av.dayOfWeek   = 2;    // Wednesday
    av.startMinute = 540;  // 09:00
    av.endMinute   = 1020; // 17:00
    const int avId = availRepo.insert(av);
    REQUIRE(avId > 0);

    // getById
    auto fetched = availRepo.getById(avId);
    REQUIRE(fetched.has_value());
    CHECK(fetched->id          == avId);
    CHECK(fetched->employeeId  == empId);
    CHECK(fetched->dayOfWeek   == 2);
    CHECK(fetched->startMinute == 540);
    CHECK(fetched->endMinute   == 1020);

    // getForEmployee
    const auto list = availRepo.getForEmployee(empId);
    REQUIRE(list.size() == 1);
    CHECK(list[0].id == avId);

    // update
    fetched->startMinute = 600;  // 10:00
    REQUIRE(availRepo.update(*fetched));
    auto updated = availRepo.getById(avId);
    REQUIRE(updated.has_value());
    CHECK(updated->startMinute == 600);

    // remove
    REQUIRE(availRepo.remove(avId));
    CHECK_FALSE(availRepo.getById(avId).has_value());
    CHECK(availRepo.getForEmployee(empId).isEmpty());

    // remove non-existent returns false
    CHECK_FALSE(availRepo.remove(avId));

    teardownConn(kConn);
}

// ── ShiftTemplateRepository CRUD ──────────────────────────────────────────────

TEST_CASE("ShiftTemplateRepository: insert, getById, getAll, update, remove", "[domain][shifttemplate]")
{
    const QString kConn = "test_shift_tmpl_crud";
    setupFullSchema(kConn);

    RoleRepository          roleRepo(kConn);
    ShiftTemplateRepository shiftRepo(kConn);

    Role r; r.name = "Cashier"; r.colourHex = "#abcdef";
    const int roleId = roleRepo.insert(r);
    REQUIRE(roleId > 0);

    // insert
    ShiftTemplate st;
    st.name          = "Morning";
    st.dayOfWeek     = 0;    // Monday
    st.startMinute   = 480;  // 08:00
    st.endMinute     = 960;  // 16:00
    st.roleId        = roleId;
    st.staffRequired = 2;
    const int stId = shiftRepo.insert(st);
    REQUIRE(stId > 0);

    // getById
    auto fetched = shiftRepo.getById(stId);
    REQUIRE(fetched.has_value());
    CHECK(fetched->id           == stId);
    CHECK(fetched->name         == "Morning");
    CHECK(fetched->dayOfWeek    == 0);
    CHECK(fetched->startMinute  == 480);
    CHECK(fetched->endMinute    == 960);
    CHECK(fetched->roleId       == roleId);
    CHECK(fetched->staffRequired == 2);

    // getAll
    const auto all = shiftRepo.getAll();
    REQUIRE(all.size() == 1);
    CHECK(all[0].id == stId);

    // update
    fetched->name          = "Early Morning";
    fetched->staffRequired = 3;
    REQUIRE(shiftRepo.update(*fetched));
    auto updated = shiftRepo.getById(stId);
    REQUIRE(updated.has_value());
    CHECK(updated->name          == "Early Morning");
    CHECK(updated->staffRequired == 3);

    // remove
    REQUIRE(shiftRepo.remove(stId));
    CHECK_FALSE(shiftRepo.getById(stId).has_value());
    CHECK(shiftRepo.getAll().isEmpty());

    // remove non-existent returns false
    CHECK_FALSE(shiftRepo.remove(stId));

    teardownConn(kConn);
}
