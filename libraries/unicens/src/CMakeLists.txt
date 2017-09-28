FILE(GLOB MyCSources *.c)
add_library (ucs2 STATIC ${MyCSources})

target_include_directories (ucs2 
	PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}/../cfg
	${CMAKE_CURRENT_SOURCE_DIR}/../inc
)
