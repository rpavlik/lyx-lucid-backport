#
#  based on cmake file
#
if (ZLIB_INCLUDE_DIR)
  # Already in cache, be silent
  set(ZLIB_FIND_QUIETLY TRUE)
endif (ZLIB_INCLUDE_DIR)

FIND_PATH(ZLIB_INCLUDE_DIR zlib.h
 /usr/include
 /usr/local/include
)

set(POTENTIAL_Z_LIBS z zlib zdll)
FIND_LIBRARY(ZLIB_LIBRARY NAMES ${POTENTIAL_Z_LIBS}
PATHS
 "C:\\Programme\\Microsoft Visual Studio 8\\VC\\lib"
 /usr/lib
 /usr/local/lib
)

IF (ZLIB_INCLUDE_DIR AND ZLIB_LIBRARY)
   SET(ZLIB_FOUND TRUE)
ENDIF (ZLIB_INCLUDE_DIR AND ZLIB_LIBRARY)

IF (ZLIB_FOUND)
   IF (NOT ZLIB_FIND_QUIETLY)
      MESSAGE(STATUS "Found Z: ${ZLIB_LIBRARY}")
   ENDIF (NOT ZLIB_FIND_QUIETLY)
ELSE (ZLIB_FOUND)
   IF (ZLIB_FIND_REQUIRED)
      MESSAGE(STATUS "Looked for Z libraries named ${POTENTIAL_Z_LIBS}.")
      MESSAGE(STATUS "Found no acceptable Z library. This is fatal.")
      MESSAGE(FATAL_ERROR "Could NOT find z library")
   ENDIF (ZLIB_FIND_REQUIRED)
ENDIF (ZLIB_FOUND)

MARK_AS_ADVANCED(ZLIB_LIBRARY ZLIB_INCLUDE_DIR)
