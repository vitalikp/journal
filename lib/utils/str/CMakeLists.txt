# journal str utils library cmake file


# tests
if (${TESTS_ENABLE})

# test test-str-empty
add_executable(test-str-empty
	test-empty.c
)

# test test-str-eq
add_executable(test-str-eq
	test-eq.c
)

# test test-str-caseeq
add_executable(test-str-caseeq
	test-caseeq.c
)

# test test-str-copy
add_executable(test-str-copy
	test-copy.c
)

add_test(NAME utils-str-empty COMMAND ./test-str-empty)
add_test(NAME utils-str-eq COMMAND ./test-str-eq)
add_test(NAME utils-str-caseeq COMMAND ./test-str-caseeq)
add_test(NAME utils-str-copy COMMAND ./test-str-copy)

endif()