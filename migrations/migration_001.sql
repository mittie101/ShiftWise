-- ShiftWise migration_001
-- B1:  Times stored as INTEGER minutes from midnight
-- B7:  weekStart always Monday (ISO date TEXT)
-- B8:  Roles matched by INTEGER id only
-- B10: foreign_keys enforced by DatabaseManager, not here
-- slotIndex: enables deterministic multi-staff slot representation

CREATE TABLE IF NOT EXISTS schema_version (
    version INTEGER PRIMARY KEY
);

INSERT OR IGNORE INTO schema_version(version) VALUES (1);

CREATE TABLE IF NOT EXISTS roles (
    id        INTEGER PRIMARY KEY,
    name      TEXT    NOT NULL UNIQUE,
    colourHex TEXT    NOT NULL DEFAULT '#e94560'
);

CREATE TABLE IF NOT EXISTS employees (
    id                INTEGER PRIMARY KEY,
    name              TEXT    NOT NULL,
    maxWeeklyMinutes  INTEGER NOT NULL DEFAULT 2400,
    priority          INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS employee_roles (
    employeeId INTEGER NOT NULL,
    roleId     INTEGER NOT NULL,
    PRIMARY KEY (employeeId, roleId),
    FOREIGN KEY (employeeId) REFERENCES employees(id) ON DELETE CASCADE,
    FOREIGN KEY (roleId)     REFERENCES roles(id)     ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS availability (
    id          INTEGER PRIMARY KEY,
    employeeId  INTEGER NOT NULL,
    dayOfWeek   INTEGER NOT NULL,  -- 0=Mon 1=Tue ... 6=Sun
    startMinute INTEGER NOT NULL,  -- minutes from midnight; endMinute > startMinute enforced by app (v1: no midnight crossing)
    endMinute   INTEGER NOT NULL,
    FOREIGN KEY (employeeId) REFERENCES employees(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS shift_templates (
    id            INTEGER PRIMARY KEY,
    name          TEXT,
    dayOfWeek     INTEGER NOT NULL,  -- 0=Mon ... 6=Sun
    startMinute   INTEGER NOT NULL,
    endMinute     INTEGER NOT NULL,  -- endMinute <= startMinute means midnight crossing
    roleId        INTEGER NOT NULL,
    staffRequired INTEGER NOT NULL DEFAULT 1,
    FOREIGN KEY (roleId) REFERENCES roles(id)
);

CREATE TABLE IF NOT EXISTS schedules (
    id         INTEGER PRIMARY KEY,
    weekStart  TEXT    NOT NULL,  -- ISO date of Monday e.g. "2025-01-06"; always Monday
    createdAt  TEXT,
    UNIQUE (weekStart)
);

CREATE TABLE IF NOT EXISTS assignments (
    id               INTEGER PRIMARY KEY,
    scheduleId       INTEGER NOT NULL,
    shiftTemplateId  INTEGER NOT NULL,
    slotIndex        INTEGER NOT NULL DEFAULT 0,  -- 0..(staffRequired-1)
    employeeId       INTEGER NOT NULL,
    isLocked         INTEGER NOT NULL DEFAULT 0,  -- 1 = skip on re-generate
    FOREIGN KEY (scheduleId)      REFERENCES schedules(id)        ON DELETE CASCADE,
    FOREIGN KEY (shiftTemplateId) REFERENCES shift_templates(id),
    FOREIGN KEY (employeeId)      REFERENCES employees(id),
    UNIQUE (scheduleId, shiftTemplateId, slotIndex)
);

-- Performance indexes (spec B18)
CREATE INDEX IF NOT EXISTS idx_availability_emp_day     ON availability(employeeId, dayOfWeek);
CREATE INDEX IF NOT EXISTS idx_shift_templates_day_start ON shift_templates(dayOfWeek, startMinute);
CREATE INDEX IF NOT EXISTS idx_assignments_schedule     ON assignments(scheduleId);
CREATE INDEX IF NOT EXISTS idx_employee_roles_role      ON employee_roles(roleId);
