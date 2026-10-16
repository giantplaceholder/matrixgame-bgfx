# Cross-compile a 64-bit Windows build from Linux.
# The original game DLL is 32-bit; this toolchain is for the FPC port and standalone EXE.
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# MinGW ships two runtime flavours: the win32 synchronous one and the posix one
# built on winpthreads. This engine uses std::thread and bgfx worker threads, so
# it needs the posix model. The distro default (update-alternatives) may point at
# either, so prefer the explicitly suffixed compiler when it exists instead of
# trusting it. Select the other one with -DMG_MINGW_THREADS=win32.
set(MG_MINGW_THREADS "posix" CACHE STRING "MinGW thread model: posix or win32")
set_property(CACHE MG_MINGW_THREADS PROPERTY STRINGS posix win32)

# An explicit -DCMAKE_C_COMPILER/-DCMAKE_CXX_COMPILER still wins: a plain set()
# here silently overrides the cache, so only fill in what is missing.
find_program(MG_MINGW_GCC NAMES x86_64-w64-mingw32-gcc-${MG_MINGW_THREADS} x86_64-w64-mingw32-gcc)
find_program(MG_MINGW_GXX NAMES x86_64-w64-mingw32-g++-${MG_MINGW_THREADS} x86_64-w64-mingw32-g++)
if(NOT CMAKE_C_COMPILER)
    set(CMAKE_C_COMPILER "${MG_MINGW_GCC}")
endif()
if(NOT CMAKE_CXX_COMPILER)
    set(CMAKE_CXX_COMPILER "${MG_MINGW_GXX}")
endif()
message(STATUS "mingw64 toolchain: ${CMAKE_CXX_COMPILER} (${MG_MINGW_THREADS} threads)")

set(CMAKE_AR          x86_64-w64-mingw32-ar         CACHE FILEPATH "" FORCE)
set(CMAKE_RANLIB      x86_64-w64-mingw32-ranlib     CACHE FILEPATH "" FORCE)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
