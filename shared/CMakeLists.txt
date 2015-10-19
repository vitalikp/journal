# journal shared cmake file 

# journal shared objects
add_library(journal_shared_obj STATIC
	special.h
	device-nodes.c
	device-nodes.h
	util.c
	util.h
	virt.c
	virt.h
	path-util.c
	path-util.h
	time-util.c
	time-util.h
	hashmap.c
	hashmap.h
	siphash24.c
	siphash24.h
	set.c
	set.h
	prioq.c
	prioq.h
	strv.c
	strv.h
	env-util.c
	env-util.h
	strbuf.c
	strbuf.h
	strxcpyx.c
	strxcpyx.h
	conf-parser.c
	conf-parser.h
	log.c
	log.h
	utf8.c
	utf8.h
	gunicode.c
	gunicode.h
	pager.c
	pager.h
	conf-files.c
	conf-files.h
	cgroup-util.c
	cgroup-util.h
	unit-name.c
	unit-name.h
	utmp-wtmp.c
	utmp-wtmp.h
	replace-var.c
	replace-var.h
	fileio.c
	fileio.h
	mkdir.c
	mkdir.h
	smack-util.c
	smack-util.h
	audit.c
	audit.h
	bus-label.c
	bus-label.h
)

add_library(journal_logs_obj STATIC
	logs-show.c
	logs-show.h
)

add_library(journal_label_obj STATIC
	label.c
	label.h
	selinux-util.c
	selinux-util.h
	mkdir-label.c
#	dev-setup.c
#	dev-setup.h
)
target_link_libraries(journal_label_obj ${SELINUX_LIBRARIES} ${SMACK_LIBRARIES})

add_library(journal_capability_obj STATIC
	capability.c
	capability.h
)
target_link_libraries(journal_capability_obj ${CAP_LIBRARIES})