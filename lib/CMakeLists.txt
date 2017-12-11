# libjournal cmake file

set(LIBJOURNAL_LDFLAGS "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/journal.sym -Wl,-z,relro")

#set(JOURNAL_CURRENT 11)
#set(JOURNAL_REVISION 5)
#set(JOURNAL_AGE 11)

# (c-a).(a).(r)
set(JOURNAL_SOVERSION 0)
set(JOURNAL_VERSION ${JOURNAL_SOVERSION}.11.5)

configure_file(journal.pc.cmake journal.pc @ONLY)


# journal utils
add_subdirectory(utils)

# journal hash
include_directories(hash)
add_subdirectory(hash)

# journal mmap
add_subdirectory(mmap)

# journal internal
set(journal_int_src
	utils.h
	sd-journal.c
	journal.h
	journal-file.c
	journal-file.h
	journal-vacuum.c
	journal-vacuum.h
	journal-send.c
	journal-def.h
	compress.c
	compress.h
)
add_library(journal_int_obj STATIC
	${journal_int_src}
)
target_link_libraries(journal_int_obj journal_utils_obj journal_hash_obj journal_mmap_obj)
target_link_libraries(journal_int_obj ${XZ_LIBRARIES} ${LZ4_LIBRARIES})
target_link_libraries(journal_int_obj -pthread rt)

add_library(journal SHARED
	${journal_int_src}
)
set_target_properties(journal PROPERTIES
	VERSION ${JOURNAL_VERSION}
	SOVERSION ${JOURNAL_SOVERSION}
)
target_link_libraries(journal journal_utils_obj journal_hash_obj journal_mmap_obj journal_shared_obj ${LIBJOURNAL_LDFLAGS})
target_link_libraries(journal ${XZ_LIBRARIES} ${LZ4_LIBRARIES})
target_link_libraries(journal -pthread rt)

# install
install(FILES journal.h DESTINATION ${includedir}/journal)
install(FILES ${PROJECT_BINARY_DIR}/lib/journal.pc DESTINATION ${libdir}/pkgconfig)
install(TARGETS journal DESTINATION ${libdir})