cmake_minimum_required(VERSION 3.10)
project(mu-update-daemon C)

set(CMAKE_C_STANDARD 99)

# Find GLib and GIO packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED gio-2.0>=2.60 glib-2.0>=2.60 gio-unix-2.0)
pkg_check_modules(SYSTEMD REQUIRED libsystemd)

# Define paths for generated code and D-Bus XML
set(GENERATED_DIR ${CMAKE_SOURCE_DIR}/src/generated)
set(GENERATED_SRC ${GENERATED_DIR}/mu_update.c)
set(GENERATED_HDR ${GENERATED_DIR}/mu_update.h)
set(DBUS_XML ${CMAKE_SOURCE_DIR}/ipc/org.mu.Update.xml)

# Generate D-Bus code
add_custom_command(
    OUTPUT ${GENERATED_SRC} ${GENERATED_HDR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${GENERATED_DIR}
    COMMAND gdbus-codegen
            --generate-c-code ${GENERATED_DIR}/mu_update
            --c-namespace MuUpdate
            ${DBUS_XML}
    DEPENDS ${DBUS_XML}
    COMMENT "Generating D-Bus interface code with gdbus-codegen"
)

add_custom_target(generate_dbus_code ALL
    DEPENDS ${GENERATED_SRC} ${GENERATED_HDR}
)

# Define the executable
add_executable(mu-update-daemon
    src/main.c
    ${GENERATED_SRC}
)

add_dependencies(mu-update-daemon generate_dbus_code)

# Include directories
target_include_directories(mu-update-daemon PRIVATE
    ${GLIB_INCLUDE_DIRS}
    ${SYSTEMD_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/generated
)

# Compiler flags
target_compile_options(mu-update-daemon PRIVATE
    ${GLIB_CFLAGS_OTHER}
)

# Link libraries
target_link_libraries(mu-update-daemon PRIVATE
    ${GLIB_LIBRARIES}
    ${SYSTEMD_LIBRARIES}
)

# Define minimum GLib version (optional but safe)
add_definitions(-DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_60)
