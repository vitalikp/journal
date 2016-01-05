# journal hash library cmake file


# tests
if (${TESTS_ENABLE})

# test hashword
add_executable(test-hashword
	hashword.c
)
target_compile_definitions(test-hashword PRIVATE TESTS)

# test hashword2
add_executable(test-hashword2
	hashword2.c
)
target_compile_definitions(test-hashword2 PRIVATE TESTS)

# test hashlittle
add_executable(test-hashlittle
	hashlittle.c
)
target_compile_definitions(test-hashlittle PRIVATE TESTS)

# test hashlittle2
add_executable(test-hashlittle2
	hashlittle2.c
)
target_compile_definitions(test-hashlittle2 PRIVATE TESTS)

# test hashbig
add_executable(test-hashbig
	hashbig.c
)
target_compile_definitions(test-hashbig PRIVATE TESTS)

add_test(NAME lookup3-hashword COMMAND ./test-hashword)
add_test(NAME lookup3-hashword2 COMMAND ./test-hashword2)
add_test(NAME lookup3-hashlittle COMMAND ./test-hashlittle)
add_test(NAME lookup3-hashlittle2 COMMAND ./test-hashlittle2)
add_test(NAME lookup3-hashbig COMMAND ./test-hashbig)

endif()
