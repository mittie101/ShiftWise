cmake_minimum_required(VERSION 3.21)
project(ShiftWise VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Suppress Qt6's optional Vulkan search (not used by this app)
set(CMAKE_DISABLE_FIND_PACKAGE_WrapVulkanHeaders TRUE)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql PrintSupport)

# ── Tests ────────────────────────────────────────────────────────────────────
add_subdirectory(tests)

# ── Main executable ──────────────────────────────────────────────────────────
add_executable(ShiftWise WIN32
    resources/resources.qrc

    src/app/main.cpp

    src/app/MainWindow.h
    src/app/MainWindow.cpp

    src/app/AppSettings.h
    src/app/AppSettings.cpp

    src/app/Logger.h
    src/app/Logger.cpp

    src/infra/DatabaseManager.h
    src/infra/DatabaseManager.cpp

    src/infra/MigrationRunner.h
    src/infra/MigrationRunner.cpp

    # ── Domain models ─────────────────────────────────────────────────────────
    src/domain/models/Role.h
    src/domain/models/Employee.h
    src/domain/models/Availability.h
    src/domain/models/ShiftTemplate.h
    src/domain/models/Schedule.h
    src/domain/models/Assignment.h

    # ── Domain utilities ───────────────────────────────────────────────────────
    src/domain/TimeUtils.h

    # ── Domain scheduler ───────────────────────────────────────────────────────
    src/domain/Scheduler.h
    src/domain/Scheduler.cpp

    # ── Domain repositories ────────────────────────────────────────────────────
    src/domain/repositories/RoleRepository.h
    src/domain/repositories/RoleRepository.cpp
    src/domain/repositories/EmployeeRepository.h
    src/domain/repositories/EmployeeRepository.cpp
    src/domain/repositories/AvailabilityRepository.h
    src/domain/repositories/AvailabilityRepository.cpp
    src/domain/repositories/ShiftTemplateRepository.h
    src/domain/repositories/ShiftTemplateRepository.cpp
    src/domain/repositories/ScheduleRepository.h
    src/domain/repositories/ScheduleRepository.cpp

    # ── UI pages ──────────────────────────────────────────────────────────────
    src/ui/pages/RolesPage.h
    src/ui/pages/RolesPage.cpp
    src/ui/pages/EmployeesPage.h
    src/ui/pages/EmployeesPage.cpp
    src/ui/pages/ShiftsPage.h
    src/ui/pages/ShiftsPage.cpp
    src/ui/pages/SchedulePage.h
    src/ui/pages/SchedulePage.cpp
    src/ui/pages/ReportsPage.h
    src/ui/pages/ReportsPage.cpp
    src/ui/pages/SettingsPage.h
    src/ui/pages/SettingsPage.cpp

    # ── UI dialogs ────────────────────────────────────────────────────────────
    src/ui/dialogs/RoleDialog.h
    src/ui/dialogs/RoleDialog.cpp
    src/ui/dialogs/EmployeeDialog.h
    src/ui/dialogs/EmployeeDialog.cpp
    src/ui/dialogs/ShiftTemplateDialog.h
    src/ui/dialogs/ShiftTemplateDialog.cpp
)

target_include_directories(ShiftWise PRIVATE src)

target_link_libraries(ShiftWise PRIVATE
    Qt6::Widgets
    Qt6::Sql
    Qt6::PrintSupport
)

target_compile_definitions(ShiftWise PRIVATE
    SHIFTWISE_VERSION="${PROJECT_VERSION}"
)

# ── Windows: embed version/icon resource ─────────────────────────────────────
if(WIN32)
    target_sources(ShiftWise PRIVATE resources/shiftwise.rc)
endif()
