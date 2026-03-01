cmake_minimum_required(VERSION 3.21)

enable_testing()

include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.6.0
)
FetchContent_MakeAvailable(Catch2)

find_package(Qt6 REQUIRED COMPONENTS Core Sql)

add_executable(ShiftWiseTests
    test_main.cpp

    ../src/app/Logger.cpp
    ../src/domain/repositories/RoleRepository.cpp
)

target_include_directories(ShiftWiseTests PRIVATE ../src)

target_link_libraries(ShiftWiseTests PRIVATE
    Catch2::Catch2WithMain
    Qt6::Core
    Qt6::Sql
)

include(CTest)
include(Catch)
catch_discover_tests(ShiftWiseTests)
