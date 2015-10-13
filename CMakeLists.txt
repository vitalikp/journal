# main journal cmake file
project(journal)
cmake_minimum_required(VERSION 3.0)


set(VERSION 214-dev)
set(CMAKE_C_COMPILER /usr/bin/gcc)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -pipe")
# preprocessor flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wp,-D_FORTIFY_SOURCE=2")
# warnings flags
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pedantic")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-result")
# code generation flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-common -fno-strict-aliasing -ffunction-sections -fdata-sections -fstack-protector -fstack-protector-strong -fexceptions")
# debugging and optimizations flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pie --param=ssp-buffer-size=4 -flto -ffat-lto-objects -ffast-math -g -O2 -fvisibility=hidden")
set(CMAKE_SHARED_LIBRARY_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set(CMAKE_EXE_C_FLAGS "${CMAKE_C_FLAGS} -fPIE")
set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--as-needed -Wl,--no-undefined -Wl,--gc-sections -Wl,-z,relro -Wl,-z,now -Wl,-fuse-ld=gold")
set(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed -Wl,--no-undefined -Wl,--gc-sections -Wl,-z,relro -Wl,-z,now -Wl,-fuse-ld=gold")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)


# check program and library
include(check.cmake)


# directories
set(_GNU_SOURCE 1)
set(bindir ${CMAKE_INSTALL_PREFIX}/bin)
set(sbindir ${CMAKE_INSTALL_PREFIX}/sbin)
set(includedir ${CMAKE_INSTALL_PREFIX}/include)
set(libdir ${CMAKE_INSTALL_PREFIX}/lib64)
set(datadir ${CMAKE_INSTALL_PREFIX}/share)

# config
configure_file(config.h.cmake config.h)
add_definitions(-include ${PROJECT_BINARY_DIR}/config.h)

# journal man
include(man/journal-man.cmake)
add_man(docs journalctl 1)
add_man(docs sd-journal 3)
add_man(docs sd_journal_add_match 3)
add_man(docs sd_journal_get_cursor 3)
add_man(docs sd_journal_get_cutoff_realtime_usec 3)
add_man(docs sd_journal_get_data 3)
add_man(docs sd_journal_get_fd 3)
add_man(docs sd_journal_get_realtime_usec 3)
add_man(docs sd_journal_get_usage 3)
add_man(docs journald.conf 5)
add_man(docs journal-fields 7)
add_man(docs journald 8)
add_custom_target(man ALL DEPENDS ${docs})

# source code
include_directories(include)
include_directories(shared)
include_directories(lib)
include_directories(src/core)
include_directories(src/internal)
add_subdirectory(shared)
add_subdirectory(lib)
add_subdirectory(src)

# data
add_subdirectory(data)

# tests
enable_testing()
add_subdirectory(test)

# show current config
message("   ${CMAKE_PROJECT_NAME} ${VERSION}")
message("      prefix:      ${CMAKE_INSTALL_PREFIX}")
message("     systemd:      ${SYSTEMD_ENABLE}")
message("       AUDIT:      ${AUDIT_ENABLE}")
message("     SELinux:      ${SELINUX_ENABLE}")
message("          XZ:      ${XZ_ENABLE}")