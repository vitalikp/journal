# journal cmake file


# journal core
include_directories(journald)
add_subdirectory(journald)
set(JOURNALD_GPERF "core/journald-gperf.gperf")
set(JOURNALD_GPERF_C "src/journald-gperf.c")
message("-- generate ${JOURNALD_GPERF_C}")
execute_process(
	COMMAND ${GPERF}
	INPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${JOURNALD_GPERF}
	OUTPUT_FILE ${JOURNALD_GPERF_C}
)
add_library(journal_core_obj STATIC
	core/conf-parser.c
	core/conf-parser.h
	core/journald-kmsg.c
	core/journald-kmsg.h
	core/journald-syslog.c
	core/journald-syslog.h
	core/journald-server.c
	core/journald-server.h
	core/journald-console.c
	core/journald-console.h
	core/journald-native.c
	core/journald-native.h
	core/journald-rate-limit.c
	core/journald-rate-limit.h
	core/journal-internal.h
	${PROJECT_BINARY_DIR}/${JOURNALD_GPERF_C}
)
target_link_libraries(journal_core_obj journald_core_obj journal_int_obj journal_shared_obj)
target_link_libraries(journal_core_obj -pthread rt)

# journalctl
add_subdirectory(journalctl)
