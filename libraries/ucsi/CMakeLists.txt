FILE(GLOB MyCSources *.c)
add_library (ucsi STATIC ${MyCSources})
target_include_directories (ucsi 
	PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/libraries/unicens/cfg-daemon
	${CMAKE_SOURCE_DIR}/libraries/unicens/ucs2/inc
)
target_link_libraries(ucsi
	ucs2
)
