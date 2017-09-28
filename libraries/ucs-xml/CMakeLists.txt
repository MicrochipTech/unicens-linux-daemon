FILE(GLOB MyCSources *.c)
add_library (ucsxml STATIC ${MyCSources})
target_include_directories (ucsxml 
	PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/libraries/unicens/cfg
	${CMAKE_SOURCE_DIR}/libraries/unicens/inc
	${CMAKE_SOURCE_DIR}/libraries/mxml
)
target_link_libraries(ucsxml
	mxml
)
