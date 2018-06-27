FILE(GLOB MyCSources *.c)
add_library (cdev STATIC ${MyCSources})
target_include_directories (cdev 
	PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}
)
