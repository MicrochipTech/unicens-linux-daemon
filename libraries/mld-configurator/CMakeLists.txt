FILE(GLOB MyCSources *.c)
add_library (mldc STATIC ${MyCSources})
target_include_directories (mldc 
	PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/libraries/ucs-xml
)
