# journal shared cmake file 

# journal shared objects
add_library(journal_shared_obj STATIC
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
)

add_library(journal_label_obj STATIC
	selinux-util.c
	selinux-util.h
#	dev-setup.c
#	dev-setup.h
)
target_link_libraries(journal_label_obj ${SELINUX_LIBRARIES} ${SMACK_LIBRARIES})