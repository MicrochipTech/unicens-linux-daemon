FILE(GLOB MyCSources *.c)
add_executable (xml2struct ${MyCSources})
target_include_directories (xml2struct 
	PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/libraries/unicens/cfg-daemon
	${CMAKE_SOURCE_DIR}/libraries/unicens/inc
	${CMAKE_SOURCE_DIR}/libraries/mxml
	${CMAKE_SOURCE_DIR}/libraries/ucs-xml
)
target_link_libraries(xml2struct
	ucs2 ucsxml
)
