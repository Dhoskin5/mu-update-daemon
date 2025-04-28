cmake_minimum_required(VERSION 3.10)
project(mu-update-daemon C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET gio-2.0>=2.60 glib-2.0>=2.60 gio-unix-2.0)

set(CMAKE_C_STANDARD 99)

set(GENERATED_DIR ${CMAKE_SOURCE_DIR}/src/generated)
set(GENERATED_SRC ${GENERATED_DIR}/mu_update.c)
set(GENERATED_HDR ${GENERATED_DIR}/mu_update.h)
set(DBUS_XML ${CMAKE_SOURCE_DIR}/ipc/org.mu.Update.xml)

add_custom_command(
    OUTPUT ${GENERATED_SRC} ${GENERATED_HDR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${GENERATED_DIR}
    COMMAND gdbus-codegen
            --generate-c-code ${GENERATED_DIR}/mu_update
            --c-namespace MuUpdate
            ${DBUS_XML}
    DEPENDS ${DBUS_XML}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating D-Bus interface code with gdbus-codegen"
)

add_custom_target(generate_dbus_code ALL
    DEPENDS ${GENERATED_SRC} ${GENERATED_HDR}
)

add_executable(mu-update-daemon
    src/main.c
    ${GENERATED_SRC}
)

add_dependencies(mu-update-daemon generate_dbus_code)

target_include_directories(mu-update-daemon PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/generated
)

target_link_libraries(mu-update-daemon PRIVATE PkgConfig::GLIB)
