# journal hash library cmake file


add_library(journal_hash_obj STATIC
	hashlittle2.c
	hash.h
)


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

# test hash
add_executable(test-hash
	hash.c
)
target_compile_definitions(test-hash PRIVATE TESTS)
target_link_libraries(test-hash journal_hash_obj)

add_test(NAME lookup3-hashword COMMAND ./test-hashword)
add_test(NAME lookup3-hashword2 COMMAND ./test-hashword2)
add_test(NAME lookup3-hashlittle COMMAND ./test-hashlittle)
add_test(NAME lookup3-hashlittle2 COMMAND ./test-hashlittle2)
add_test(NAME lookup3-hashbig COMMAND ./test-hashbig)
add_test(NAME journal-hash COMMAND ./test-hash)

endif()
