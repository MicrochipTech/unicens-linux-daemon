FILE(GLOB MyCSources *.c)
add_library (mxml STATIC ${MyCSources})
target_include_directories (mxml 
	PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}
)
