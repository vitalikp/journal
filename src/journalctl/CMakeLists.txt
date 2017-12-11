# journalctl cmake file


add_executable(journalctl
	logs-show.c
	logs-show.h
	journal-verify.c
	journal-verify.h
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
	journal-verify.c
)
target_compile_definitions(test-journal-verify PRIVATE TESTS)
target_link_libraries(test-journal-verify journal_core_obj)

add_test(NAME journal-verify COMMAND ./test-journal-verify)

endif()