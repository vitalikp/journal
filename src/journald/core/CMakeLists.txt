# journald core cmake file


# journald core
add_library(journald_core_obj STATIC
	epollfd.c
	epollfd.h
	socket.c
	socket.h
	server.c
	server.h
)
target_link_libraries(journald_core_obj journal_int_obj journal_shared_obj)
target_link_libraries(journald_core_obj -pthread rt)