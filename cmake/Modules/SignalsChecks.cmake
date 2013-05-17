
check_include_file(signal.h HAVE_SIGNAL_H)

IF ( HAVE_SIGNAL_H )
CHECK_C_SOURCE_COMPILES("#include <signal.h>\n int main(void) { signal(SIGPIPE, 0); }" HAVE_SIGPIPE)
CHECK_C_SOURCE_COMPILES("#include <signal.h>\n int main(void) { signal(SIGBUS, 0); }"  HAVE_SIGBUS)
CHECK_C_SOURCE_COMPILES("#include <signal.h>\n int main(void) { signal(SIGIO, 0); }"   HAVE_SIGIO)
CHECK_C_SOURCE_COMPILES("#include <signal.h>\n int main(void) { signal(SIGPOLL, 0); }" HAVE_SIGPOLL)
CHECK_C_SOURCE_COMPILES("#include <signal.h>\n int main(void) { signal(SIGQUIT, 0); }" HAVE_SIGQUIT)
CHECK_C_SOURCE_COMPILES("#include <signal.h>\n int main(void) { signal(SIGINT, 0); }"  HAVE_SIGINT)
CHECK_C_SOURCE_COMPILES("#include <signal.h>\n int main(void) { signal(SIGTERM, 0); }" HAVE_SIGTERM)
CHECK_C_SOURCE_COMPILES("#include <signal.h>\n int main(void) { signal(SIGHUP, 0); }"  HAVE_SIGHUP)
CHECK_C_SOURCE_COMPILES("#include <signal.h>\n int main(void) { signal(SIGHUP, 0); }"  HAVE_SIGILL)
ENDIF ( HAVE_SIGNAL_H )
