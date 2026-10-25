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

file(GLOB_RECURSE PV_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

if(PV_SOURCES)
    add_library(engine STATIC ${PV_SOURCES})
    set(PV_SCOPE PUBLIC)
    target_link_libraries(engine
        PUBLIC  raylib
        PRIVATE pv_warnings
    )
endif()

target_include_directories(engine
    ${PV_SCOPE}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)


file(GLOB_RECURSE PV_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
)

target_sources(engine
    ${PV_SCOPE}
        FILE_SET pv_headers TYPE HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
        FILES ${PV_HEADERS}
)

set_target_properties(engine PROPERTIES VERIFY_INTERFACE_HEADER_SETS ON)

# ---------------------------------------------------------------------------
# Executable
# ---------------------------------------------------------------------------

add_executable(engine_e
    app/main.cpp
)

target_link_libraries(engine_e
    PRIVATE
        engine
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