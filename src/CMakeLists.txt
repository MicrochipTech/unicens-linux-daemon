FILE(GLOB MyCSources *.c)
add_executable (unicensd ${MyCSources})
target_include_directories (unicensd 
	PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/libraries/unicens/cfg-daemon
	${CMAKE_SOURCE_DIR}/libraries/unicens/ucs2/inc
	${CMAKE_SOURCE_DIR}/libraries/mxml
	${CMAKE_SOURCE_DIR}/libraries/ucs-xml
)
find_package (Threads)
target_link_libraries(unicensd
	ucs2 ucsxml ${CMAKE_THREAD_LIBS_INIT} ${ADDITIONAL_PLATFORM_LIBS}
)

CHECK_LIBRARY_EXISTS(rt timer_settime "time.h" NEED_LIBRT)
if(${NEED_LIBRT})
	TARGET_LINK_LIBRARIES(unicensd rt)
endif(${NEED_LIBRT})
