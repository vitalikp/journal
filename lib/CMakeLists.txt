# libjournal cmake file

set(LIBJOURNAL_C_FLAGS "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/journal.sym -Wl,-z,relro")

#set(JOURNAL_CURRENT 11)
#set(JOURNAL_REVISION 5)
#set(JOURNAL_AGE 11)

# (c-a).(a).(r)
set(JOURNAL_SOVERSION 0)
set(JOURNAL_VERSION ${JOURNAL_SOVERSION}.11.5)

configure_file(journal.pc.cmake journal.pc @ONLY)

# journal internal
set(journal_int_src
	uuid.c
	uuid.h
	sd-event.c
	sd-event.h
	sd-journal.c
	journal.h
	journal-file.c
	journal-file.h
	journal-vacuum.c
	journal-vacuum.h
	journal-verify.c
	journal-verify.h
	lookup3.c
	lookup3.h
	journal-send.c
	journal-def.h
	compress.h
	mmap-cache.c
	mmap-cache.h
)
if (${HAVE_XZ})
	list(APPEND journal_int_src compress.c)
endif()
add_library(journal_int_obj STATIC
	${journal_int_src}
)
target_link_libraries(journal_int_obj ${XZ_LIBRARIES} ${SYSTEMD_LIBRARIES})
target_link_libraries(journal_int_obj -pthread rt)

add_library(journal SHARED
	${journal_int_src}
)
set_target_properties(journal PROPERTIES
	VERSION ${JOURNAL_VERSION}
	SOVERSION ${JOURNAL_SOVERSION}
)
target_link_libraries(journal journal_shared_obj ${LIBJOURNAL_C_FLAGS})
target_link_libraries(journal ${XZ_LIBRARIES} ${SYSTEMD_LIBRARIES})
target_link_libraries(journal -pthread rt)

# install
install(FILES journal.h DESTINATION ${includedir}/journal)
install(FILES ${PROJECT_BINARY_DIR}/lib/journal.pc DESTINATION ${libdir}/pkgconfig)
install(TARGETS journal DESTINATION ${libdir})