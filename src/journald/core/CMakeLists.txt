# journald core cmake file


# journald core
add_library(journald_core_obj STATIC
	run/mkdir.c
	run/group.c
	run/user.c
	run/chgroup.c
	run/chuser.c
	epollfd.c
	epollfd.h
	msg/new.c
	msg/resize.c
	msg/decode.c
	msg.h
	cmsg/decode.c
	cmsg.h
	hostname/open.c
	hostname/read.c
	hostname.h
	socket/open.c
	socket/setsndbuf.c
	socket/getsize.c
	socket/sendmsg.c
	socket/recvmsg.c
	socket.h
	syslog/open.c
	syslog/close.c
	syslog/run.c
	syslog.h
	native/open.c
	native/close.c
	native.h
	seqnum/load.c
	seqnum/save.c
	seqnum.h
	kmsg/close.c
	kmsg/read.c
	kmsg/decode.c
	kmsg.h
	server/start.c
	server/stop.c
	server/run.c
	server.h
)
target_link_libraries(journald_core_obj journal_int_obj journal_shared_obj)
target_link_libraries(journald_core_obj -pthread rt)