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