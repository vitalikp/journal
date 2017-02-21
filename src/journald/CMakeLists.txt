# journald cmake file


# journald core
add_subdirectory(core)

# journald
add_executable(journald
	journald.c
)
target_link_libraries(journald journald_core_obj journal_core_obj journal_int_obj journal_shared_obj)

# install
install(TARGETS journald DESTINATION ${sbindir})

# tests
if (${TESTS_ENABLE})

# test-epolfd
add_executable(test-epollfd
	test-epollfd.c
)
target_link_libraries(test-epollfd journal_core_obj)

add_test(NAME journald-epollfd COMMAND ./test-epollfd)

endif()
