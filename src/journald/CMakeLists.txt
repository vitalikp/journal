# journald cmake file


# journald
add_executable(journald
	journald.c
)
target_link_libraries(journald journal_core_obj journal_int_obj journal_shared_obj)

# install
install(TARGETS journald DESTINATION ${sbindir})
