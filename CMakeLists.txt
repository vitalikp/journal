# main journal cmake file
project(journal)
cmake_minimum_required(VERSION 3.0)


set(VERSION 214.3-dev)
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
set(logdir /var/log)

# config
configure_file(config.h.cmake config.h)
add_definitions(-include ${PROJECT_BINARY_DIR}/config.h)

# journal man
include(man/journal-man.cmake)
add_man(docs 1 journalctl)
add_man(docs 3 sd-journal)
add_man(docs 3 sd_journal_add_match)
add_man_install(3 sd_journal_add_conjunction)
add_man_install(3 sd_journal_add_disjunction)
add_man_install(3 sd_journal_flush_matches)
add_man(docs 3 sd_journal_get_cursor)
add_man_install(3 sd_journal_test_cursor)
add_man(docs 3 sd_journal_get_cutoff_realtime_usec)
add_man_install(3 sd_journal_get_cutoff_monotonic_usec)
add_man(docs 3 sd_journal_get_data)
add_man_install(3 sd_journal_enumerate_data)
add_man_install(3 sd_journal_restart_data)
add_man_install(3 sd_journal_set_data_threshold)
add_man_install(3 sd_journal_get_data_threshold)
add_man(docs 3 sd_journal_get_fd)
add_man_install(3 sd_journal_get_events)
add_man_install(3 sd_journal_get_timeout)
add_man_install(3 sd_journal_process)
add_man_install(3 sd_journal_wait)
add_man_install(3 sd_journal_reliable_fd)
add_man(docs 3 sd_journal_get_realtime_usec)
add_man_install(3 sd_journal_get_monotonic_usec)
add_man(docs 3 sd_journal_get_usage)
add_man(docs 3 sd_journal_next)
add_man_install(3 sd_journal_previous)
add_man_install(3 sd_journal_next_skip)
add_man_install(3 sd_journal_previous_skip)
add_man(docs 3 sd_journal_open)
add_man_install(3 sd_journal_open_directory)
add_man_install(3 sd_journal_open_files)
add_man_install(3 sd_journal_close)
add_man_install(3 sd_journal)
add_man(docs 3 sd_journal_print)
add_man_install(3 sd_journal_printv)
add_man_install(3 sd_journal_send)
add_man_install(3 sd_journal_sendv)
add_man_install(3 sd_journal_perror)
add_man(docs 3 sd_journal_query_unique)
add_man_install(3 sd_journal_enumerate_unique)
add_man_install(3 sd_journal_restart_unique)
add_man(docs 3 sd_journal_seek_head)
add_man_install(3 sd_journal_seek_tail)
add_man_install(3 sd_journal_seek_monotonic_usec)
add_man_install(3 sd_journal_seek_realtime_usec)
add_man_install(3 sd_journal_seek_cursor)
add_man(docs 5 journald.conf)
add_man(docs 7 journal-fields)
add_man(docs 8 journald)
add_custom_target(man ALL DEPENDS ${docs})

# source code
add_definitions(-DJOURNAL_COMPILATION)
include_directories(include)
include_directories(shared)
include_directories(lib)
include_directories(src/core)
add_subdirectory(shared)
add_subdirectory(lib)
add_subdirectory(src)

# data
add_subdirectory(data)

# tests
if (${TESTS_ENABLE})
add_subdirectory(test)
endif()

# show current config
message("   ${CMAKE_PROJECT_NAME} ${VERSION}")
message("      prefix:      ${CMAKE_INSTALL_PREFIX}")
message("       tests:      ${TESTS_ENABLE}")
message("       AUDIT:      ${AUDIT_ENABLE}")
message("     SELinux:      ${SELINUX_ENABLE}")
message("          XZ:      ${XZ_ENABLE}")