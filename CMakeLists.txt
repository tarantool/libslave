cmake_minimum_required(VERSION 2.8)
project(libslave)

set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(BOOST_DIR ${INCLUDE_DIR})
set(MYSQL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/mysql-src")

include_directories(
	BEFORE SYSTEM ${INCLUDE_DIR}
)
include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	"${BOOST_DIR}/boost_1_59_0/boost"
)

set(COPTS_FLAGS "-pipe -fomit-frame-pointer -ffunction-sections -fdata-sections")

add_custom_target(
	mysql_configure
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
		-DCMAKE_C_FLAGS=${COPTS_FLAGS} -DCMAKE_CXX_FLAGS=${COPTS_FLAGS}

	WORKING_DIRECTORY ${MYSQL_DIR}
)
add_custom_target(
	mysql_headers_target
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${MYSQL_DIR}/include" ${INCLUDE_DIR}
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${MYSQL_DIR}/include" "${INCLUDE_DIR}/mysql"
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${MYSQL_DIR}/libbinlogevents/export" ${INCLUDE_DIR}
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${MYSQL_DIR}/libbinlogevents/export" "${INCLUDE_DIR}/mysql"
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${MYSQL_DIR}/zlib" ${INCLUDE_DIR}
	DEPENDS mysql_configure
	WORKING_DIRECTORY ${MYSQL_DIR}
)

add_custom_target(
	mysqlclient_target
	COMMAND ${CMAKE_COMMAND} --build . --target mysqlclient
	DEPENDS mysql_configure
	WORKING_DIRECTORY ${MYSQL_DIR}
)
add_library(mysqlclient STATIC IMPORTED)
set_property(TARGET mysqlclient PROPERTY IMPORTED_LOCATION "${MYSQL_DIR}/libmysql/libmysqlclient.a")
add_dependencies(mysqlclient mysqlclient_target)

add_custom_target(
	binlogevents_target
	COMMAND ${CMAKE_COMMAND} --build . --target binlogevents_static
	DEPENDS mysql_configure
	WORKING_DIRECTORY ${MYSQL_DIR}
)
add_library(binlogevents STATIC IMPORTED)
set_property(TARGET binlogevents PROPERTY IMPORTED_LOCATION "${MYSQL_DIR}/libbinlogevents/lib/libbinlogevents.a")
add_dependencies(binlogevents binlogevents_target)


file(GLOB HDR "*.h")
install(FILES ${HDR} DESTINATION include)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SRC)

add_library(slave STATIC ${SRC})
set_target_properties(slave PROPERTIES COMPILE_FLAGS "-std=c++11" OUTPUT_NAME slave)
add_dependencies(slave mysql_headers_target)

include("${MYSQL_DIR}/cmake/libutils.cmake")
MERGE_STATIC_LIBS(slave_a slave_a "mysqlclient;binlogevents;slave")

install(TARGETS slave_a DESTINATION lib64)
