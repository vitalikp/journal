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
	fileio.c
	fileio.h
)