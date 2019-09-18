CMAKE_MINIMUM_REQUIRED (VERSION 3.1.3)
PROJECT (libslave)

OPTION (BUILD_STATIC "Force building static library" ON)
OPTION (WITH_TESTING "Enable building the tests framework" OFF)

# Build flags
SET (CMAKE_CXX_STANDARD 11)
SET (CMAKE_CXX_STANDARD_REQUIRED TRUE)
SET (CMAKE_CXX_EXTENSIONS FALSE)

IF (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
 AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9)
    MESSAGE (FATAL_ERROR "libslave requires GCC version >= 4.9 for proper regex support")
ENDIF ()

IF (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
 OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Common options
    ADD_DEFINITIONS (-pipe)
    ADD_DEFINITIONS (-Wall)
ENDIF ()

# Fixes a lot of annoying warnings about auto_ptr deprecation
ADD_DEFINITIONS (-DBOOST_NO_AUTO_PTR)

SET (BOOST_DIR ${CMAKE_CURRENT_SOURCE_DIR})
SET (MYSQL_SRC ${CMAKE_CURRENT_SOURCE_DIR}/mysql)
SET (MYSQL_BIN ${CMAKE_BINARY_DIR}/mysql)
FILE (MAKE_DIRECTORY ${MYSQL_BIN})

INCLUDE_DIRECTORIES (
    BEFORE SYSTEM
    ${MYSQL_BIN}/include
    ${MYSQL_SRC}/include
    ${MYSQL_SRC}/libbinlogevents/export
    ${BOOST_DIR}/boost_1_59_0/boost
)
ADD_CUSTOM_TARGET (
    mysql_configure
    WORKING_DIRECTORY ${MYSQL_BIN}
    COMMAND "${CMAKE_COMMAND}"
        -DCMAKE_BUILD_TYPE=Release
        -DWITH_RAPID=0
        -DDISABLE_SHARED=1
        -DENABLED_PROFILING=0
        -DWITHOUT_SERVER=1
        -DWITH_CLIENT_PROTOCOL_TRACING=0
        -DWITH_PIC=0
        -DWITH_DEFAULT_FEATURE_SET=0
        -DWITH_SSL=bundled
        -DWITH_ZLIB=bundled
        -DWITH_LIBEVENT=bundled
        -DWITH_LZ4=bundled
        -DWITH_PROTOBUF=bundled
        -DWITH_EDITLINE=bundled
        -DDOWNLOAD_BOOST=1 -DWITH_BOOST=${BOOST_DIR}
        ${MYSQL_SRC}
)
ADD_CUSTOM_TARGET (
    mysqlclient_build
    WORKING_DIRECTORY ${MYSQL_BIN}
    DEPENDS mysql_configure
    COMMAND ${CMAKE_COMMAND} --build . --target mysqlclient
)
ADD_CUSTOM_TARGET (
    binlogevents_build
    WORKING_DIRECTORY ${MYSQL_BIN}
    DEPENDS mysql_configure
    COMMAND ${CMAKE_COMMAND} --build . --target binlogevents_static
)

ADD_LIBRARY (mysqlclient STATIC IMPORTED)
ADD_DEPENDENCIES (mysqlclient mysqlclient_build)
SET_PROPERTY (TARGET mysqlclient PROPERTY IMPORTED_LOCATION ${MYSQL_BIN}/libmysql/libmysqlclient.a)

ADD_LIBRARY (binlogevents STATIC IMPORTED)
ADD_DEPENDENCIES (binlogevents binlogevents_build)
SET_PROPERTY (TARGET binlogevents PROPERTY IMPORTED_LOCATION ${MYSQL_BIN}/libbinlogevents/lib/libbinlogevents.a)

ADD_DEFINITIONS (-DDBUG_OFF)
SET (LINK_TYPE STATIC)
SET (MYSQL_LIBS mysqlclient binlogevents)
MESSAGE (STATUS "Build ${LINK_TYPE} slave library")

FILE (GLOB HDR "*.h")
INSTALL (FILES ${HDR} DESTINATION include)
AUX_SOURCE_DIRECTORY (${CMAKE_CURRENT_SOURCE_DIR} SRC)

# Most probably static mysql is built without fPIC, so, we can't build dynamic library with it
ADD_LIBRARY (slave ${LINK_TYPE} ${SRC})
TARGET_LINK_LIBRARIES (slave ${MYSQL_LIBS} -lpthread)
INSTALL (TARGETS slave DESTINATION lib64)

IF (WITH_TESTING)
    IF (Boost_FOUND)
        ENABLE_TESTING ()
    ENDIF()
    ADD_SUBDIRECTORY (test)
ENDIF()
