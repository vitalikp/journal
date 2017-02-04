# journal mmap library cmake file


add_library(journal_mmap_obj STATIC
	mmap-cache.c
	mmap-cache.h
)
target_link_libraries(journal_mmap_obj journal_shared_obj)


# tests
if (${TESTS_ENABLE})

# test-mmap-cache
add_executable(test-mmap-cache
	test-mmap-cache.c
)
target_link_libraries(test-mmap-cache journal_mmap_obj)

add_test(NAME mmap-cache COMMAND ./test-mmap-cache)

endif()
