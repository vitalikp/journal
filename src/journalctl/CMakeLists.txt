# journalctl cmake file


add_executable(journalctl
	logs-show.c
	logs-show.h
	journalctl.c
)
target_link_libraries(journalctl journal_int_obj journal_shared_obj)
#target_link_libraries(journalctl -pthread rt)

# install
install(TARGETS journalctl DESTINATION ${bindir})

# tests
if (${TESTS_ENABLE})

# test-journal-verify
add_executable(test-journal-verify
	test-journal-verify.c
)
target_link_libraries(test-journal-verify journal_core_obj)

add_test(NAME journal-verify COMMAND ./test-journal-verify)

endif()