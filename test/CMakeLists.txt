# journal tests cmake file

# test-journal
add_executable(test-journal
	test-journal.c
)
target_link_libraries(test-journal journal_core_obj)

# test-journal-enum
add_executable(test-journal-enum
	test-journal-enum.c
)
target_link_libraries(test-journal-enum journal_core_obj)

# test-journal-flush
add_executable(test-journal-flush
	test-journal-flush.c
)
target_link_libraries(test-journal-flush journal_core_obj)

# test-journal-init
add_executable(test-journal-init
	test-journal-init.c
)
target_link_libraries(test-journal-init journal_core_obj)

# test-journal-interleaving
add_executable(test-journal-interleaving
	test-journal-interleaving.c
)
target_link_libraries(test-journal-interleaving journal_core_obj)

# test-journal-match
add_executable(test-journal-match
	test-journal-match.c
)
target_link_libraries(test-journal-match journal_core_obj)

# test-journal-send
add_executable(test-journal-send
	test-journal-send.c
)
target_link_libraries(test-journal-send journal_core_obj)

# test-journal-stream
add_executable(test-journal-stream
	test-journal-stream.c
)
target_link_libraries(test-journal-stream journal_core_obj)

# test-compress
add_executable(test-compress
	test-compress.c
)
target_link_libraries(test-compress journal_int_obj journal_shared_obj)

# test-compress-benchmark
add_executable(test-compress-benchmark
	test-compress-benchmark.c
)
target_link_libraries(test-compress-benchmark journal_int_obj journal_shared_obj)

add_test(NAME journal COMMAND ./test-journal)
add_test(NAME journal-enum COMMAND ./test-journal-enum)
add_test(NAME journal-flush COMMAND ./test-journal-flush)
add_test(NAME journal-init COMMAND ./test-journal-init)
add_test(NAME journal-interleaving COMMAND ./test-journal-interleaving)
add_test(NAME journal-match COMMAND ./test-journal-match)
add_test(NAME journal-stream COMMAND ./test-journal-stream)
add_test(NAME journal-compress COMMAND ./test-compress)
add_test(NAME journal-compress-benchmark COMMAND ./test-compress-benchmark)
