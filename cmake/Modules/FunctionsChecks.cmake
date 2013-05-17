
INCLUDE (CheckFunctionExists)

check_function_exists(readv		HAVE_READV)
check_function_exists(writev		HAVE_WRITEV)
check_function_exists(mmap		HAVE_MMAP)
check_function_exists(getloadavg	HAVE_GETLOADAVG)
check_function_exists(getenv		HAVE_GETENV)
check_function_exists(setenv		HAVE_SETENV)
check_function_exists(putenv		HAVE_PUTENV)
check_function_exists(vasprintf		HAVE_VASPRINTF)
check_function_exists(strndup		HAVE_STRNDUP)
check_function_exists(strcasestr	HAVE_STRCASESTR)
check_function_exists(strcasecmp	HAVE_STRCASECMP)
check_function_exists(utime		HAVE_UTIME)
check_function_exists(utimes		HAVE_UTIMES)

