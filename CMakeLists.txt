cmake_minimum_required(VERSION 3.24)   # FILE_SET HEADERS + header-set verification

project(Engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(pv_warnings INTERFACE)

find_package(raylib REQUIRED)

target_compile_options(pv_warnings
    INTERFACE
        -Wall
        -Wextra
        -Wpedantic
        -Wnon-virtual-dtor
)

# ---------------------------------------------------------------------------
# Layers
#
#   engine     core physics (bodies, colliders, broad/narrow phase, world)
#   runtime    harness: window, main loop, rendering, timing, run logging,
#              the Scenario interface              -> depends on engine
#   scenarios  concrete test scenarios + registry  -> depends on runtime
#
# Each layer lives in <layer>/include/<layer>/ and <layer>/src/, so includes
# read as "engine/World.h", "runtime/Application.h", ...
# Sources and headers are globbed: drop a file in and reconfigure.
# ---------------------------------------------------------------------------

function(engine_add_layer name)
    set(dir ${CMAKE_CURRENT_SOURCE_DIR})

    file(GLOB_RECURSE sources CONFIGURE_DEPENDS ${dir}/src/*.cpp)
    file(GLOB_RECURSE headers CONFIGURE_DEPENDS ${dir}/include/*.h)

    add_library(${name} STATIC ${sources})

    target_sources(${name}
        PUBLIC
            FILE_SET HEADERS
            BASE_DIRS ${dir}/include
            FILES ${headers}
    )

    target_link_libraries(${name} PRIVATE pv_warnings)

    set_target_properties(${name} PROPERTIES VERIFY_INTERFACE_HEADER_SETS ON)
endfunction()

add_subdirectory(engine)
add_subdirectory(runtime)
add_subdirectory(scenarios)

# ---------------------------------------------------------------------------
# Executable
# ---------------------------------------------------------------------------

add_executable(engine_e
    app/main.cpp
)

target_link_libraries(engine_e
    PRIVATE
        scenarios
        pv_warnings
)

# ---------------------------------------------------------------------------
# Sandbox
#
# Standalone prototypes (each has its own main()). Not part of the engine.
# ---------------------------------------------------------------------------

option(ENGINE_BUILD_SANDBOX "Build the standalone prototypes in sandbox/" OFF)

if(ENGINE_BUILD_SANDBOX)
    file(GLOB PV_SANDBOX_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/sandbox/*.cpp
    )

    foreach(src ${PV_SANDBOX_SOURCES})
        get_filename_component(name ${src} NAME_WE)
        add_executable(${name} ${src})
        target_link_libraries(${name} PRIVATE pv_warnings raylib)
    endforeach()
endif()

# ---------------------------------------------------------------------------
# Tests
#
# Also globbed: drop a file in tests/ and it is picked up on the next configure.
# ---------------------------------------------------------------------------

# include(CTest)

# if(BUILD_TESTING)

#     file(GLOB_RECURSE PV_TEST_SOURCES CONFIGURE_DEPENDS
#         ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp
#     )

#     if(PV_TEST_SOURCES)

#         include(FetchContent)

#         FetchContent_Declare(
#             googletest
#             GIT_REPOSITORY https://github.com/google/googletest.git
#             GIT_TAG v1.17.0
#         )

#         FetchContent_MakeAvailable(googletest)

#         add_executable(provenance_tests ${PV_TEST_SOURCES})

#         target_link_libraries(provenance_tests
#             PRIVATE
#                 provenance
#                 pv_warnings
#                 GTest::gtest_main
#         )

#         include(GoogleTest)

#         gtest_discover_tests(provenance_tests)

#     else()
#         message(STATUS "provenance: no test sources in tests/ yet -- skipping test target")
#     endif()

# endif()