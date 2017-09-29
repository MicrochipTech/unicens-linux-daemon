FILE(GLOB MyCSources ucs2/src/*.c)
add_library (ucs2 STATIC ${MyCSources})

target_include_directories (ucs2 
	PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}/cfg-daemon
	${CMAKE_CURRENT_SOURCE_DIR}/ucs2/inc
)
