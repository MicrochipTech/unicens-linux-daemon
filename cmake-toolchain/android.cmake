set(CMAKE_SYSTEM_NAME  Android)
set(CMAKE_SYSTEM_VERSION 1)

set(CMAKE_C_COMPILER /tmp/arm-linux-androideabi/bin/arm-linux-androideabi-gcc)
set(CMAKE_CXX_COMPILER /tmp/arm-linux-androideabi/bin/arm-linux-androideabi-g++)

UNSET(CMAKE_EXE_LINKER_FLAGS CACHE)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC" CACHE STRING "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -pie" CACHE STRING "" FORCE)

set(ADDITIONAL_PLATFORM_LIBS "log")
set(ADDITIONAL_PLATFORM_FLAGS "-DANDROID")
