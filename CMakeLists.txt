cmake_minimum_required(VERSION 3.20)
project(RosePan LANGUAGES CXX)

# PC port of Rule of Rose (US, SLUS_214.48), reconstructed function by function. The
# platform layer (src/rosepan, src/sdk) comes from ReiPan, the Fatal Frame 3 port, which
# took it from MioPan (Fatal Frame 2); game code under src/ is Rule of Rose's own.

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(WIN32 AND NOT MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "-static")
endif()

set(ROSEPAN_ICON "${CMAKE_CURRENT_SOURCE_DIR}/extern/rosepan.ico")
if(WIN32 AND EXISTS "${ROSEPAN_ICON}")
    set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/extern/icon.rc")
endif()

set(BUILD_SHARED_LIBS OFF)
list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/RosePan)

# The prebuilt FFmpeg and shadercross trees are not committed here; when extern/ lacks
# them, cmake/ffmpeg.cmake and cmake/shadercross.cmake reuse a sibling checkout's: ReiPan's
# when it holds them, else MioPan's.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../ReiPan/extern/ffmpeg")
    set(_rosepan_sibling_extern "${CMAKE_CURRENT_SOURCE_DIR}/../ReiPan/extern")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../MioPan/extern")
    set(_rosepan_sibling_extern "${CMAKE_CURRENT_SOURCE_DIR}/../MioPan/extern")
else()
    set(_rosepan_sibling_extern "${CMAKE_CURRENT_SOURCE_DIR}/../MioPan-Mikompilation/extern")
endif()
set(ROSEPAN_SIBLING_EXTERN "${_rosepan_sibling_extern}"
        CACHE PATH "extern/ of a ReiPan or MioPan checkout, used for FFmpeg/shadercross when RosePan has none.")

include(sdl3)
include(spdlog)
include(cglm)
include(imgui)
include(implot)
include(shadercross)
include(ffmpeg)

file(GLOB_RECURSE ROSEPAN_TRANSLATION_UNITS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
)
file(GLOB_RECURSE ROSEPAN_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.inc"
)
set(ROSEPAN_SOURCES ${ROSEPAN_TRANSLATION_UNITS} ${ROSEPAN_HEADERS})
set_source_files_properties(${ROSEPAN_HEADERS} PROPERTIES HEADER_FILE_ONLY TRUE)

set(ROSEPAN_INCLUDE_DIRS
    src
    src/sdk
)

# Rule of Rose is C++ (GCC 2.96 classes and vtables) and the platform layer is C++, so
# every translation unit builds as C++.
set_source_files_properties(${ROSEPAN_TRANSLATION_UNITS} PROPERTIES LANGUAGE CXX)

add_executable(RosePan ${ROSEPAN_SOURCES} ${APP_ICON_RESOURCE_WINDOWS})
add_dependencies(RosePan compile_shaders)
target_include_directories(RosePan PRIVATE ${ROSEPAN_INCLUDE_DIRS})
target_link_libraries(RosePan PRIVATE
    SDL3::SDL3
    spdlog::spdlog
    cglm
    imgui::imgui
    implot::implot
)

# The MPEG-2 decoder behind sceMpegGetPicture(); see cmake/ffmpeg.cmake. Without the
# prebuilt tree, movies play their audio over a black screen.
if(ROSEPAN_HAVE_FFMPEG)
    target_compile_definitions(RosePan PRIVATE ROSEPAN_HAVE_FFMPEG=1)
    target_include_directories(RosePan PRIVATE "${FFMPEG_INCLUDE_DIR}")
    target_link_libraries(RosePan PRIVATE avcodec avutil swscale)
    if(WIN32)
        add_custom_command(TARGET RosePan POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${ROSEPAN_FFMPEG_ROOT}/bin"
                    "$<TARGET_FILE_DIR:RosePan>"
        )
    endif()
endif()

add_custom_command(TARGET RosePan POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/resources"
            "$<TARGET_FILE_DIR:RosePan>/resources"
)

# ---- build stamp for the log banner -------------------------------------
# Read at configure time and scoped to rosepan_log.cpp so a new commit rebuilds only it.
set(ROSEPAN_BUILD_COMMIT "unknown")
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse --short=12 HEAD
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE ROSEPAN_GIT_COMMIT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" status --porcelain --untracked-files=no
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE ROSEPAN_GIT_DIRTY
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(ROSEPAN_GIT_COMMIT)
        set(ROSEPAN_BUILD_COMMIT "${ROSEPAN_GIT_COMMIT}")
        if(NOT ROSEPAN_GIT_DIRTY STREQUAL "")
            string(APPEND ROSEPAN_BUILD_COMMIT "-dirty")
        endif()
    endif()
endif()
message(STATUS "Build stamp: ${ROSEPAN_BUILD_COMMIT}")
set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/rosepan/io/rosepan_log.cpp"
    PROPERTIES COMPILE_DEFINITIONS
        "ROSEPAN_BUILD_COMMIT=\"${ROSEPAN_BUILD_COMMIT}\";ROSEPAN_BUILD_TYPE=\"${CMAKE_BUILD_TYPE}\""
)

# Force-include the EE base-type shim into every TU so u_char/u_int/u_long (64-bit) and
# NULL are ambient exactly as the PS2 SDK's ubiquitous includes made them.
set(ROSEPAN_PREFIX_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/src/sdk/pc_prefix.h")
if(MSVC)
    target_compile_options(RosePan PRIVATE "/FI${ROSEPAN_PREFIX_HEADER}")
else()
    target_compile_options(RosePan PRIVATE -include "${ROSEPAN_PREFIX_HEADER}")
endif()

# ---- aliasing ------------------------------------------------------------
# NOT OPTIONAL. Reconstructed code reads the game's file formats in place and walks DMA
# packets through u_int*; GCC/Clang exploit the strict-aliasing violation from -O2 up
# (MioPan's release-only crash in gra3dsgdDrawPresetDataObject). MSVC needs nothing.
if(NOT MSVC)
    target_compile_options(RosePan PRIVATE -fno-strict-aliasing)
endif()

# ---- buffer-overflow checks in Debug --------------------------------------
# Reconstructed code keeps the game's own overruns; fortifying Debug makes them abort
# under a debugger. It needs optimisation, hence -Og (-DROSEPAN_DEBUG_FORTIFY=OFF for -O0).
option(ROSEPAN_DEBUG_FORTIFY
       "Build the GCC/Clang Debug config at -Og with _FORTIFY_SOURCE=3" ON)
if(NOT MSVC AND ROSEPAN_DEBUG_FORTIFY)
    target_compile_options(RosePan PRIVATE $<$<CONFIG:Debug>:-Og>)
    target_compile_definitions(RosePan PRIVATE $<$<CONFIG:Debug>:_FORTIFY_SOURCE=3>)
endif()

# ---- host-compiler quirks ----------------------------------------------
if(MSVC)
    target_compile_options(RosePan PRIVATE
        /permissive
        /W1
        /wd4996
        /D_CRT_SECURE_NO_WARNINGS
        /Zc:preprocessor
    )
else()
    target_compile_options(RosePan PRIVATE -w -fpermissive)
endif()
