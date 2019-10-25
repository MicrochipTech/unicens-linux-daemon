SET(CMAKE_SYSTEM_NAME QNX)
SET(CMAKE_SYSTEM_VERSION 700)
SET(arch armv7)

SET(QNX_HOST ${QNX_BASE}/host/linux/x86_64)
SET(QNX_TARGET ${QNX_BASE}/target/qnx7)

SET(CMAKE_MAKE_PROGRAM "${QNX_HOST}/usr/bin/make"   CACHE PATH "QNX make Program")
SET(CMAKE_SH           "${QNX_HOST}/usr/bin/sh"     CACHE PATH "QNX shell Program")
SET(CMAKE_AR           "${QNX_HOST}/usr/bin/nto${arch}-ar"        CACHE PATH "QNX ar Program")
SET(CMAKE_RANLIB       "${QNX_HOST}/usr/bin/nto${arch}-ranlib"    CACHE PATH "QNX ranlib Program")
SET(CMAKE_NM           "${QNX_HOST}/usr/bin/nto${arch}-nm"        CACHE PATH "QNX nm Program")
SET(CMAKE_OBJCOPY      "${QNX_HOST}/usr/bin/nto${arch}-objcopy"   CACHE PATH "QNX objcopy Program")
SET(CMAKE_OBJDUMP      "${QNX_HOST}/usr/bin/nto${arch}-objdump"   CACHE PATH "QNX objdump Program")
SET(CMAKE_LINKER       "${QNX_HOST}/usr/bin/nto${arch}-ld"        CACHE PATH "QNX linker Program")
SET(CMAKE_STRIP        "${QNX_HOST}/usr/bin/nto${arch}-strip"     CACHE PATH "QNX strip Program")

SET(CMAKE_C_COMPILER    ${QNX_HOST}/usr/bin/nto${arch}-gcc)
SET(CMAKE_CXX_COMPILER  ${QNX_HOST}/usr/bin/nto${arch}-g++)

SET(CMAKE_FIND_ROOT_PATH ${QNX_TARGET})
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
