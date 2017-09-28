FILE(GLOB MyCSources *.c)
add_executable (unicensd ${MyCSources})
target_include_directories (unicensd 
	PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/libraries/unicens/cfg
	${CMAKE_SOURCE_DIR}/libraries/unicens/inc
	${CMAKE_SOURCE_DIR}/libraries/mxml
	${CMAKE_SOURCE_DIR}/libraries/ucs-xml
)
find_package (Threads)
find_library (unicensd rt)
target_link_libraries(unicensd
	ucs2 ucsxml ${CMAKE_THREAD_LIBS_INIT} rt
)
