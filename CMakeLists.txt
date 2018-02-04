# main journal cmake file
project(journal)
cmake_minimum_required(VERSION 3.0)


set(VERSION 214.3-dev)
set(VER 0.1-dev)
if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_C_COMPILER /usr/bin/gcc)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -pipe")
# preprocessor flags
set(CMAKE_C_FLAGS_RELEASE "-Wp,-D_FORTIFY_SOURCE=2")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-Wp,-D_FORTIFY_SOURCE=2")
# warnings flags
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pedantic")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wformat=2 -Wno-unused-result")
# code generation flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-common -fno-strict-aliasing -ffunction-sections -fdata-sections -fstack-protector -fstack-protector-strong -fexceptions")
# debugging and optimizations flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pie --param=ssp-buffer-size=4 -flto -ffat-lto-objects -ffast-math -fvisibility=hidden")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2 -DNDEBUG")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -g -O2 -DNDEBUG")
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
if (NOT SYSCONF_INSTALL_DIR)
	set(SYSCONF_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/etc")
endif()
set(JOURNAL_SYSCONFDIR ${SYSCONF_INSTALL_DIR}/journal)
set(bindir ${CMAKE_INSTALL_PREFIX}/bin)
set(sbindir ${CMAKE_INSTALL_PREFIX}/sbin)
set(includedir ${CMAKE_INSTALL_PREFIX}/include)
set(libdir ${CMAKE_INSTALL_PREFIX}/lib64)
set(JOURNAL_RUNDIR /run/journal)
set(datadir ${CMAKE_INSTALL_PREFIX}/share)
set(logdir /var/log)

# config
configure_file(config.h.cmake config.h)
add_definitions(-include ${PROJECT_BINARY_DIR}/config.h)

# journal man
include(man/journal-man.cmake)
add_man(docs 1 journalctl)
add_man(docs 3 sd-journal)
add_man(docs 3
	sd_journal_add_match
	sd_journal_add_conjunction
	sd_journal_add_disjunction
	sd_journal_flush_matches
)
add_man(docs 3
	sd_journal_get_cursor
	sd_journal_test_cursor
)
add_man(docs 3
	sd_journal_get_cutoff_realtime_usec
	sd_journal_get_cutoff_monotonic_usec
)
add_man(docs 3
	sd_journal_get_data
	sd_journal_enumerate_data
	sd_journal_restart_data
	sd_journal_set_data_threshold
	sd_journal_get_data_threshold
)
add_man(docs 3
	sd_journal_get_fd
	sd_journal_get_events
	sd_journal_process
	sd_journal_wait
)
add_man(docs 3
	sd_journal_get_realtime_usec
	sd_journal_get_monotonic_usec
)
add_man(docs 3 sd_journal_get_usage)
add_man(docs 3
	sd_journal_next
	sd_journal_previous
	sd_journal_next_skip
	sd_journal_previous_skip
)
add_man(docs 3
	sd_journal_open
	sd_journal_open_directory
	sd_journal_open_files
	sd_journal_close
	sd_journal
)
add_man(docs 3
	sd_journal_print
	sd_journal_printv
	sd_journal_send
	sd_journal_sendv
	sd_journal_perror
)
add_man(docs 3
	sd_journal_query_unique
	sd_journal_enumerate_unique
	sd_journal_restart_unique
)
add_man(docs 3
	sd_journal_seek_head
	sd_journal_seek_tail
	sd_journal_seek_monotonic_usec
	sd_journal_seek_realtime_usec
	sd_journal_seek_cursor
)
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
message("   ${CMAKE_PROJECT_NAME} ${VERSION}(${CMAKE_BUILD_TYPE})")
message("      prefix:      ${CMAKE_INSTALL_PREFIX}")
message("     sysconf:      ${SYSCONF_INSTALL_DIR}")
message("       tests:      ${TESTS_ENABLE}")
message("          XZ:      ${XZ_ENABLE}")
message("         LZ4:      ${LZ4_ENABLE}")