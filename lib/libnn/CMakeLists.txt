
CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/nn_conf.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/nn_conf.h
)

SET( LIBNN_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE INTERNAL "")
SET( LIBNN_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}" CACHE INTERNAL "")

include_directories (
    ${APR_INCLUDE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/include
    include
)

SET (NN_SOURCES
	nn_buffer.c
	nn_log.c
	nn_xml.c
	nn_utils.c
)

add_library (
    nn-static
	${NN_SOURCES}
    )

SET (LINKLIBS
    ${APR_LIBRARY}
)

target_link_libraries (
    nn-static
	${LINKLIBS}
)
