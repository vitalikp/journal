# journal utils library cmake file


include_directories(.)
add_subdirectory(str)

add_library(journal_utils_obj STATIC
	parse-uint.c
	uuid/equal.c
	uuid/isnull.c
	uuid/genrand.c
	uuid/tostr.c
	uuid/parse.c
	uuid.h
	boot/getid.c
	boot.h
)


# tests
if (${TESTS_ENABLE})

# test parse-uint
add_executable(test-parse-uint
	parse-uint.c
)
target_compile_definitions(test-parse-uint PRIVATE TESTS)

add_test(NAME utils-parse-uint COMMAND ./test-parse-uint)

endif()
