# check program and library cmake file

include(CheckTypeSize)
include(CheckFunctionExists)
include(CheckLibraryExists)

# check ln
find_program(LN ln)
if (NOT LN)
	message(FATAL_ERROR "*** ln not found! ***")
else()
	message("-- Found ln")
endif()

# check zsh
find_program(ZSH zsh)
if (ZSH)
	message("-- Found zsh")
endif()

# check xsltproc
find_program(XSLTPROC xsltproc)
if (NOT XSLTPROC)
	message(FATAL_ERROR "*** xsltproc not found! ***")
else()
	message("-- Found xsltproc")
endif()

# check awk
find_program(AWK awk)
if (NOT AWK)
	message(FATAL_ERROR "*** awk not found! ***")
else()
	EXECUTE_PROCESS(COMMAND ${AWK} -V OUTPUT_VARIABLE AWK_VERSION)
	string(REGEX REPLACE "^GNU Awk ([^\n]+)\n.*" "\\1" AWK_VERSION "${AWK_VERSION}")
	message("-- Found awk ${AWK_VERSION}")
endif()

# check gperf
find_program(GPERF gperf)
if (NOT GPERF)
	message(FATAL_ERROR "*** gperf not found! ***")
else()
	EXECUTE_PROCESS(COMMAND ${GPERF} -v OUTPUT_VARIABLE GPERF_VERSION)
	string(REGEX REPLACE "^GNU gperf ([^\n]+)\n.*" "\\1" GPERF_VERSION "${GPERF_VERSION}")
	message("-- Found gperf ${GPERF_VERSION}")
endif()

# check type size
check_type_size(pid_t SIZEOF_PID_T)
check_type_size(uid_t SIZEOF_UID_T)
check_type_size(gid_t SIZEOF_GID_T)
check_type_size(time_t SIZEOF_TIME_T)
set(CMAKE_EXTRA_INCLUDE_FILES sys/time.h sys/resource.h)
check_type_size(rlim_t SIZEOF_RLIM_T)

# check headers
check_include_file(sys/auxv.h HAVE_SYS_AUXV_H)

# check functions
CHECK_FUNCTION_EXISTS(__secure_getenv HAVE___SECURE_GETENV)
CHECK_FUNCTION_EXISTS(secure_getenv HAVE_SECURE_GETENV)
set(CMAKE_EXTRA_INCLUDE_FILES fcntl.h)
set(CMAKE_EXTRA_INCLUDE_FILES sched.h)

# dependencies
find_package(PkgConfig REQUIRED)

# check bash-completion
pkg_check_modules(BASH_COMPL bash-completion)

# check tests option
option(TESTS_ENABLE "Enable build tests" OFF)
if (${TESTS_ENABLE})
	enable_testing()
endif()

# check xz library
option(XZ_ENABLE "Disable optional XZ support" ON)
if (${XZ_ENABLE})
	pkg_check_modules(XZ REQUIRED liblzma)
	set(HAVE_XZ 1)
endif()

# check lz4 library
option(LZ4_ENABLE "Enable optional LZ4 support" OFF)
if (${LZ4_ENABLE})
	pkg_check_modules(LZ4 REQUIRED liblz4)
	set(HAVE_LZ4 1)
endif()

# get sys_uid_max
EXECUTE_PROCESS(
	COMMAND ${AWK} "BEGIN { uid=999 } /^\\s*SYS_UID_MAX\\s+/ { uid=$2 } END { printf uid }"
	INPUT_FILE /etc/login.defs
	OUTPUT_VARIABLE SYSTEM_UID_MAX)
