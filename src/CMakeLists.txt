
CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/nproxyd.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/nproxyd.h
)

include_directories(
    ./include
    ${CMAKE_CURRENT_BINARY_DIR}
    ${APR_INCLUDE_DIR}
    ${LIBNN_SOURCE_DIR}/include
    ${LIBNN_BINARY_DIR}/include
    ${LIBNPROXY_SOURCE_DIR}/include
)

link_directories(
    ${LIBNN_BINARY_DIR}
    ${LIBNPROXY_BINARY_DIR}
)

SET(NPROXY_SOURCES
    nproxyd_log.c
    nproxyd.c
)

add_executable(
    "nproxyd"
	${NPROXY_SOURCES}
)

target_link_libraries (
    nproxyd
	nproxy
)

SET_TARGET_PROPERTIES(
    nproxyd
    PROPERTIES
	OUTPUT_NAME nproxyd
	INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib/nproxy"
)

IF (NOT APPLE AND NOT WIN32)
SET_TARGET_PROPERTIES(
    nproxyd
    PROPERTIES
	LINK_FLAGS "${NPROXY_LINKER_OPTIONS}"
)
ENDIF (NOT APPLE AND NOT WIN32)

INSTALL(
    TARGETS 
	nproxyd
    RUNTIME DESTINATION ${SBIN_INSTALL_DIR}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

