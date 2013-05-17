
CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/nproxy-conf.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/nproxy-conf.h
)

SET( LIBNPROXY_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE INTERNAL "")
SET( LIBNPROXY_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}" CACHE INTERNAL "")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}
    ${LIBNN_SOURCE_DIR}/include
    ${LIBNN_BINARY_DIR}/include
    ${APR_INCLUDE_DIR}
)

SET(NPROXY_SOURCES
    libnproxy.h
    libnproxy.c
    libnproxy_profiles.c
    libnproxy_conn.c
    libnproxy_response.c
    libnproxy_headers.c
    libnproxy_auth.c
    libnproxy_utils.c
)

SET (LINKLIBS
	nn-static
	${APR_LIBRARY}
)

IF ( WIN32 )
    SET (
        LINKLIBS
	    ${LINKLIBS}
	    ws2_32.lib
    )
ENDIF ( WIN32 )


IF ( LUA_FOUND )
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DLUA_VERSION_NUMBER=${LUA_VER}" )

    include_directories(
	${LUA_INCLUDE_DIR}
    )

    SET(
	NPROXY_SOURCES  
	    ${NPROXY_SOURCES}
	    libnproxy_lua.c
	    libnproxy_swig.h
	    libnproxy_swig.c
	    libnproxy_swig_wrap.c
    )

    SET (
        LINKLIBS 
	    ${LINKLIBS} 
	    ${LUA_LIBRARIES}
    )
ENDIF ( LUA_FOUND )

add_library (
    nproxy STATIC
	${NPROXY_SOURCES}
    )

target_link_libraries (
    nproxy
    ${LINKLIBS}
)


SET_TARGET_PROPERTIES(
    nproxy
    PROPERTIES
	SOVERSION 1
    PROJECT_LABEL "NProxy Library"
)

IF (NOT APPLE AND NOT WIN32)
SET_TARGET_PROPERTIES(
    nproxy
    PROPERTIES
	LINK_FLAGS "${NPROXY_LINKER_OPTIONS}"
)
ENDIF (NOT APPLE AND NOT WIN32)


