	if(LEVELDB_INCLUDE_DIR AND LEVELDB_LIBRARIES)
	   set(LEVELDB_FOUND TRUE)
	
	else(LEVELDB_INCLUDE_DIR AND LEVELDB_LIBRARIES)
	
	 FIND_PATH(LEVELDB_INCLUDE_DIR leveldb/c.h
				/usr/include
	      /usr/local/include/
	      )
	
	  find_library(LEVELDB_LIBRARIES NAMES leveldb libleveldb
	     PATHS
	     /usr/lib
	     /usr/local/lib
       /usr/lib64
       /usr/local/lib64
	     /usr/lib/x86_64-linux-gnu
	     )
	     
	  if(LEVELDB_INCLUDE_DIR AND LEVELDB_LIBRARIES)
	    set(LEVELDB_FOUND TRUE)
	    message(STATUS "Found LevelDB: ${LEVELDB_INCLUDE_DIR}, ${LEVELDB_LIBRARIES}")
	    INCLUDE_DIRECTORIES(${LEVELDB_INCLUDE_DIR})
	  else(LEVELDB_INCLUDE_DIR AND LEVELDB_LIBRARIES)
	    set(LEVELDB_FOUND FALSE)
	    message(STATUS "LevelDB not found.")
	  endif(LEVELDB_INCLUDE_DIR AND LEVELDB_LIBRARIES)
	
	  mark_as_advanced(LEVELDB_INCLUDE_DIR LEVELDB_LIBRARIES)
	
	endif(LEVELDB_INCLUDE_DIR AND LEVELDB_LIBRARIES)

