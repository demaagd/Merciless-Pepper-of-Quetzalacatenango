project (urlshortd)
cmake_minimum_required (VERSION 2.4)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")

include (mysql.cmake)
include (postgresql.cmake)
include (leveldb.cmake)

find_package (PkgConfig REQUIRED)
pkg_check_modules (SQLITE3 REQUIRED sqlite3)

include_directories (${SQLITE3_INCLUDE_DIRS} ${MYSQL_INCLUDE_DIR} ${POSTGRESQL_INCLUDE_DIR})
link_directories (${SQLITE3_LIBRARY_DIRS})
add_definitions (${SQLITE3_CFLAGS_OTHER})

if(SQLITE3_FOUND)
	add_library(mod_sqlite SHARED mod_sqlite.c)
	target_link_libraries (mod_sqlite ${SQLITE3_LIBRARIES})
endif(SQLITE3_FOUND)

if(MYSQL_FOUND)
	add_library(mod_mysql SHARED mod_mysql.c)
	target_link_libraries(mod_mysql ${MYSQL_LIBRARY})
endif(MYSQL_FOUND)

if(POSTGRESQL_FOUND)
	add_library(mod_postgres SHARED mod_postgres.c)
	target_link_libraries(mod_postgres ${POSTGRESQL_LIBRARIES})
endif(POSTGRESQL_FOUND)

if(LEVELDB_FOUND)
	add_library(mod_leveldb SHARED mod_leveldb.c)
	target_link_libraries(mod_leveldb ${LEVELDB_LIBRARIES})
endif(LEVELDB_FOUND)

add_executable (urlshortd urlshortd.c util.c util.h tmpldfl.h mongoose.c mongoose.h)
target_link_libraries (urlshortd dl pthread)

INSTALL(TARGETS urlshortd DESTINATION urlshortd)

SET(CPACK_GENERATOR "DEB")
SET(CPACK_GENERATOR "RPM")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Dave DeMaagd") #required

INCLUDE(CPack)
