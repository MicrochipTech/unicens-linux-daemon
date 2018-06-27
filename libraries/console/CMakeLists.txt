FILE(GLOB MyCSources *.c)
add_library (console STATIC ${MyCSources})
target_include_directories (console 
	PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}
)
