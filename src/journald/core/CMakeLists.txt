# journald core cmake file


# journald core
add_library(journald_core_obj STATIC
	run/mkdir.c
	run/group.c
	run/user.c
	epollfd.c
	epollfd.h
	socket.c
	socket.h
	syslog/close.c
	syslog.h
	native.c
	native.h
	server/start.c
	server/stop.c
	server/run.c
	server.h
)
target_link_libraries(journald_core_obj journal_int_obj journal_shared_obj)
target_link_libraries(journald_core_obj -pthread rt)