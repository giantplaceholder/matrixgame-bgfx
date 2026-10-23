cmake_minimum_required(VERSION 3.21)
project(matrixgame LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# exe, dll, or both, one configure so bgfx is built once
set(MG_TARGET "exe" CACHE STRING "Build kind: exe, dll or both")
set_property(CACHE MG_TARGET PROPERTY STRINGS exe dll both)
if(NOT MG_TARGET MATCHES "^(exe|dll|both)$")
    message(FATAL_ERROR "MG_TARGET must be exe, dll or both (got '${MG_TARGET}')")
endif()

if(WIN32)
    if(NOT MG_SDL2_ROOT)
        message(FATAL_ERROR "Set -DMG_SDL2_ROOT to the SDL2 MinGW development tree (the directory that contains x86_64-w64-mingw32).")
    endif()
    set(MG_SDL2_ARCH "${MG_SDL2_ROOT}/x86_64-w64-mingw32")
    set(MG_SDL2_WINLINKS user32 gdi32 winmm imm32 ole32 oleaut32 shell32
        setupapi version uuid dinput8 dxguid dxerr8)
    if(MG_TARGET STREQUAL "dll" OR MG_TARGET STREQUAL "both")
        # static sdl2, system deps come along on the interface
        add_library(SDL2::SDL2-static STATIC IMPORTED)
        set_target_properties(SDL2::SDL2-static PROPERTIES
            IMPORTED_LOCATION "${MG_SDL2_ARCH}/lib/libSDL2.a"
            INTERFACE_INCLUDE_DIRECTORIES "${MG_SDL2_ARCH}/include;${MG_SDL2_ARCH}/include/SDL2"
            INTERFACE_LINK_LIBRARIES "${MG_SDL2_WINLINKS}")
    endif()
    if(NOT MG_TARGET STREQUAL "dll")
        add_library(SDL2::SDL2 SHARED IMPORTED)
        set_target_properties(SDL2::SDL2 PROPERTIES
            IMPORTED_IMPLIB "${MG_SDL2_ARCH}/lib/libSDL2.dll.a"
            IMPORTED_LOCATION "${MG_SDL2_ARCH}/bin/SDL2.dll"
            INTERFACE_INCLUDE_DIRECTORIES "${MG_SDL2_ARCH}/include;${MG_SDL2_ARCH}/include/SDL2")
    endif()
    if(NOT MG_PNG_INCLUDE OR NOT MG_PNG_LIBRARY OR NOT MG_ZLIB_INCLUDE OR NOT MG_ZLIB_LIBRARY)
        message(FATAL_ERROR "Set MG_PNG_INCLUDE, MG_PNG_LIBRARY, MG_ZLIB_INCLUDE and MG_ZLIB_LIBRARY.")
    endif()
    add_library(PNG::PNG UNKNOWN IMPORTED)
    set_target_properties(PNG::PNG PROPERTIES IMPORTED_LOCATION "${MG_PNG_LIBRARY}" INTERFACE_INCLUDE_DIRECTORIES "${MG_PNG_INCLUDE}")
    add_library(ZLIB::ZLIB UNKNOWN IMPORTED)
    set_target_properties(ZLIB::ZLIB PROPERTIES IMPORTED_LOCATION "${MG_ZLIB_LIBRARY}" INTERFACE_INCLUDE_DIRECTORIES "${MG_ZLIB_INCLUDE}")
else()
if(MG_TARGET STREQUAL "dll" OR MG_TARGET STREQUAL "both")
    # Archives must be -fPIC; the distro's are not. Scripts set MG_PIC_PREFIX
    if(NOT MG_PIC_PREFIX)
        message(FATAL_ERROR "MG_TARGET=${MG_TARGET} needs -DMG_PIC_PREFIX=<prefix of PIC static libs>")
    endif()
endif()
if(MG_PIC_PREFIX)
    set(MG_SDL2_STATIC "${MG_PIC_PREFIX}/lib/libSDL2.a")
    set(MG_SDL2_INCLUDE_DIR "${MG_PIC_PREFIX}/include/SDL2")
    set(MG_PNG "${MG_PIC_PREFIX}/lib/libpng16.a")
    set(MG_PNG_INCLUDE "${MG_PIC_PREFIX}/include")
    set(MG_Z "${MG_PIC_PREFIX}/lib/libz.a")
    set(MG_ZLIB_INCLUDE "${MG_PIC_PREFIX}/include")
    foreach(var MG_SDL2_STATIC MG_SDL2_INCLUDE_DIR MG_PNG MG_Z)
        if(NOT EXISTS "${${var}}")
            message(FATAL_ERROR "${var} not found at ${${var}}")
        endif()
    endforeach()
    # PNG/ZLIB bind to the PIC archives; find_package would pick shared libs
    set(MG_PIC_DEPS ON)
    add_library(SDL2::SDL2 STATIC IMPORTED)
    set_target_properties(SDL2::SDL2 PROPERTIES
        IMPORTED_LOCATION "${MG_SDL2_STATIC}"
        INTERFACE_INCLUDE_DIRECTORIES "${MG_SDL2_INCLUDE_DIR}")
    set(SDL2_INCLUDE_DIRS "${MG_SDL2_INCLUDE_DIR}")
    set(ENV{PKG_CONFIG_PATH} "${MG_PIC_PREFIX}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
else()
    find_package(SDL2 QUIET CONFIG)
    if(NOT TARGET SDL2::SDL2)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(PC_SDL2 REQUIRED IMPORTED_TARGET sdl2)
        add_library(SDL2::SDL2 ALIAS PkgConfig::PC_SDL2)
        set(SDL2_INCLUDE_DIRS ${PC_SDL2_INCLUDE_DIRS})
    endif()
endif()
if(MG_PIC_DEPS)
    # Bound to the PIC archives above; find_package would pick shared libraries
    add_library(PNG::PNG UNKNOWN IMPORTED)
    set_target_properties(PNG::PNG PROPERTIES
        IMPORTED_LOCATION "${MG_PNG}" INTERFACE_INCLUDE_DIRECTORIES "${MG_PNG_INCLUDE}")
    add_library(ZLIB::ZLIB UNKNOWN IMPORTED)
    set_target_properties(ZLIB::ZLIB PROPERTIES
        IMPORTED_LOCATION "${MG_Z}" INTERFACE_INCLUDE_DIRECTORIES "${MG_ZLIB_INCLUDE}")
else()
    find_package(PNG QUIET)
    if(NOT TARGET PNG::PNG)
    find_library(MG_PNG NAMES png png16)
    if(NOT MG_PNG)
        find_file(MG_PNG NAMES libpng16.so.16 libpng16.so PATHS /usr/lib /usr/lib/x86_64-linux-gnu /lib /lib/x86_64-linux-gnu)
    endif()
    if(NOT MG_PNG)
        message(FATAL_ERROR "libpng not found. Install libpng-dev.")
    endif()
    add_library(PNG::PNG UNKNOWN IMPORTED)
    set_target_properties(PNG::PNG PROPERTIES IMPORTED_LOCATION "${MG_PNG}")
    endif()
    find_package(ZLIB QUIET)
    if(NOT TARGET ZLIB::ZLIB)
    find_library(MG_Z NAMES z)
    if(NOT MG_Z)
        find_file(MG_Z NAMES libz.so.1 libz.so PATHS /usr/lib /usr/lib/x86_64-linux-gnu /lib /lib/x86_64-linux-gnu)
    endif()
    if(NOT MG_Z)
        message(FATAL_ERROR "zlib not found. Install zlib1g-dev.")
    endif()
    add_library(ZLIB::ZLIB UNKNOWN IMPORTED)
    set_target_properties(ZLIB::ZLIB PROPERTIES IMPORTED_LOCATION "${MG_Z}")
    endif()
endif()
endif()

# bx, bimg, bgfx and bgfx.cmake. The build scripts pass MG_DEPS, or -DBX_DIR and the rest
if(NOT MG_DEPS AND DEFINED ENV{MG_DEPS})
    set(MG_DEPS "$ENV{MG_DEPS}")
endif()
if(NOT MG_DEPS)
    set(MG_DEPS "${CMAKE_CURRENT_SOURCE_DIR}/../../deps")
endif()
set(MG_DEPS "${MG_DEPS}" CACHE PATH "Directory that contains bx, bimg, bgfx and bgfx.cmake")
set(BX_DIR "${MG_DEPS}/bx" CACHE PATH "")
set(BIMG_DIR "${MG_DEPS}/bimg" CACHE PATH "")
set(BGFX_DIR "${MG_DEPS}/bgfx" CACHE PATH "")
set(BGFX_CMAKE_DIR "${MG_DEPS}/bgfx.cmake" CACHE PATH "")
foreach(dep BX_DIR BIMG_DIR BGFX_DIR BGFX_CMAKE_DIR)
    if(NOT EXISTS "${${dep}}")
        message(FATAL_ERROR "${dep} not found at ${${dep}}. Run the build script or set MG_DEPS to a checkout of bx, bimg, bgfx and bgfx.cmake.")
    endif()
endforeach()
if(WIN32)
    set(BGFX_WITH_WAYLAND OFF CACHE BOOL "" FORCE)
else()
    find_library(MG_WAYLAND_EGL NAMES wayland-egl)
    if(MG_WAYLAND_EGL)
        set(BGFX_WITH_WAYLAND ON CACHE BOOL "" FORCE)
    else()
        message(STATUS "libwayland-egl not found; Wayland EGL is off. Install libwayland-dev to enable the GL fallback on Wayland.")
        set(BGFX_WITH_WAYLAND OFF CACHE BOOL "" FORCE)
    endif()
endif()
set(BGFX_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BGFX_INSTALL OFF CACHE BOOL "" FORCE)
# one bgfx archive for both the exe and the shared library
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_subdirectory(${BGFX_CMAKE_DIR} ${CMAKE_BINARY_DIR}/bgfx-cmake EXCLUDE_FROM_ALL)

if(WIN32)
    set(MG_PLAT_SRC windows/win32_shim.cpp)
else()
    set(MG_PLAT_SRC linux/linux_host.cpp)
endif()

file(GLOB GAME_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/../MatrixGame/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../MatrixGame/src/Text/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../MatrixGame/src/Interface/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../MatrixGame/src/Effects/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../MatrixGame/src/Logic/*.cpp)

file(GLOB LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../MatrixLib/*/*.cpp)
if(NOT WIN32 AND MG_PIC_DEPS)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PC_SDL2_STATIC REQUIRED sdl2)
    set(MG_SDL2_DEPS ${PC_SDL2_STATIC_STATIC_LIBRARIES})
    list(REMOVE_ITEM MG_SDL2_DEPS SDL2)
endif()
if(WIN32)
    add_library(mg_host STATIC windows/host_sdl.cpp windows/win32_host.cpp)
    target_include_directories(mg_host PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/windows
        ${MG_SDL2_ARCH}/include
        ${MG_SDL2_ARCH}/include/SDL2)
    target_link_libraries(mg_host PUBLIC bgfx bx bimg)
    target_compile_options(mg_host PRIVATE -Wno-error -Wno-unused-parameter)
endif()

function(mg_add_game kind)
    set(mg_incs
        ${CMAKE_CURRENT_SOURCE_DIR}/common/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../MatrixLib/Base
        ${CMAKE_CURRENT_SOURCE_DIR}/../MatrixLib/3G
        ${CMAKE_CURRENT_SOURCE_DIR}/../MatrixLib/Bitmap
        ${CMAKE_CURRENT_SOURCE_DIR}/../MatrixGame/src
        ${CMAKE_CURRENT_SOURCE_DIR}/../MatrixGame
        ${CMAKE_CURRENT_SOURCE_DIR}/../MatrixGame/src/Interface
        ${CMAKE_CURRENT_SOURCE_DIR}/../MatrixGame/src/Text
        ${SDL2_INCLUDE_DIRS})
    if(kind STREQUAL "dll")
        add_library(matrixgame_dll SHARED
            ${MG_PLAT_SRC} common/d3d9_bgfx.cpp common/font_stub.cpp ${GAME_SRC} ${LIB_SRC})
        set(target matrixgame_dll)
        # hosts look for MatrixGame.dll / MatrixGame.so by that name
        set_target_properties(${target} PROPERTIES
            PREFIX "" OUTPUT_NAME "MatrixGame"
            WINDOWS_EXPORT_ALL_SYMBOLS OFF
            CXX_VISIBILITY_PRESET default)
        target_compile_definitions(${target} PRIVATE ASSERT_OFF MATRIXGAME_BGFX MATRIXGAME_EXPORTS)
    else()
        add_executable(matrixgame_exe standalone/main.cpp
            ${MG_PLAT_SRC} common/d3d9_bgfx.cpp common/font_stub.cpp ${GAME_SRC} ${LIB_SRC})
        set(target matrixgame_exe)
        set_target_properties(${target} PROPERTIES OUTPUT_NAME "matrixgame")
        # both targets need MATRIXGAME_EXPORTS or the exe treats the entry as a dll import
        target_compile_definitions(${target} PRIVATE
            ASSERT_OFF MATRIXGAME_BGFX BUILD_EXE MATRIXGAME_EXPORTS)
    endif()
    target_include_directories(${target} BEFORE PRIVATE ${mg_incs})
    target_compile_options(${target} PRIVATE
        -Wno-error -Wno-unused-parameter -fpermissive -fno-strict-aliasing -fno-tree-sra -fno-tree-fre
        -include ${CMAKE_CURRENT_SOURCE_DIR}/common/include/d3dx9.h)
    # release objects still have dwarf from the static libs, strip it out of the image
    target_link_options(${target} PRIVATE -Wl,--strip-debug)
    if(WIN32)
        if(kind STREQUAL "dll")
            target_link_libraries(${target} PRIVATE mg_host SDL2::SDL2-static PNG::PNG ZLIB::ZLIB)
            target_link_options(${target} PRIVATE -static -static-libgcc -static-libstdc++)
        else()
            target_link_libraries(${target} PRIVATE mg_host SDL2::SDL2 PNG::PNG ZLIB::ZLIB)
            # -static keeps winpthread inside the exe, sdl2 stays a dll, we link libSDL2.dll.a
            target_link_options(${target} PRIVATE -static -static-libgcc -static-libstdc++)
            # per-monitor v2 manifest, or windows bitmap-scales the window
            set(MG_MANIFEST_OBJ ${CMAKE_CURRENT_BINARY_DIR}/matrixgame.manifest.obj)
            add_custom_command(
                OUTPUT ${MG_MANIFEST_OBJ}
                COMMAND ${CMAKE_RC_COMPILER} -O coff -o ${MG_MANIFEST_OBJ}
                        -I ${CMAKE_CURRENT_SOURCE_DIR}/windows
                        ${CMAKE_CURRENT_SOURCE_DIR}/windows/matrixgame.rc
                DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/windows/matrixgame.rc
                        ${CMAKE_CURRENT_SOURCE_DIR}/windows/matrixgame.manifest
                VERBATIM)
            set_source_files_properties(${MG_MANIFEST_OBJ} PROPERTIES EXTERNAL_OBJECT TRUE GENERATED TRUE)
            target_sources(${target} PRIVATE ${MG_MANIFEST_OBJ})
        endif()
    else()
        target_link_libraries(${target} PRIVATE bgfx bx bimg SDL2::SDL2 PNG::PNG ZLIB::ZLIB dl pthread)
        if(MG_PIC_DEPS)
            target_link_directories(${target} PRIVATE ${PC_SDL2_STATIC_STATIC_LIBRARY_DIRS})
            target_link_libraries(${target} PRIVATE ${MG_SDL2_DEPS})
            target_link_options(${target} PRIVATE ${PC_SDL2_STATIC_STATIC_LDFLAGS_OTHER})
        endif()
        if(kind STREQUAL "dll")
            # only export the host entry, leave the rest local
            target_link_options(${target} PRIVATE
                -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/linux/matrixgame.map
                -Wl,-Bsymbolic)
        endif()
    endif()
endfunction()

if(MG_TARGET STREQUAL "dll" OR MG_TARGET STREQUAL "both")
    mg_add_game(dll)
endif()
if(NOT MG_TARGET STREQUAL "dll")
    mg_add_game(exe)
endif()
