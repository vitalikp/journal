# journald cmake file


# journald core
add_library(journald_core_obj STATIC
	core/epollfd.c
	core/epollfd.h
	core/socket.c
	core/socket.h
)
target_link_libraries(journald_core_obj journal_int_obj journal_label_obj journal_shared_obj)
target_link_libraries(journald_core_obj ${AUDIT_LIBRARIES})
target_link_libraries(journald_core_obj -pthread rt)


# journald
add_executable(journald
	journald.c
)
target_link_libraries(journald journald_core_obj journal_core_obj journal_int_obj journal_shared_obj)

# install
install(TARGETS journald DESTINATION ${sbindir})
