# FFmpeg -- the MPEG-2 video decoder behind sceMpegGetPicture().
#
# The PSS movies on the disc are MPEG-2 (sequence_extension on every sequence
# header, picture_coding_extension on every picture, full IPB GOPs), so an
# MPEG-1 decoder cannot be used.  Only three libraries are needed:
#
#   avcodec   the decoder itself
#   avutil    its frame/buffer types
#   swscale   YUV420P -> RGBA, which is the format the IPU produced
#
# avformat and swresample are deliberately absent: sceMpegDemuxPss() in
# sdk/libmpeg.cpp is a full PSS demultiplexer already, so avcodec is fed raw
# elementary-stream bytes, and the audio is plain PCM that needs no resampling.
#
# Windows uses a prebuilt tree under extern/ffmpeg with include/, lib/ and bin/,
# laid out like MikuPan's.  Unlike MikuPan's this module does NOT hard-fail when
# the tree is absent -- it reports ROSEPAN_HAVE_FFMPEG OFF and the build continues
# with video decoding compiled out, so the project still builds for anyone who has
# not fetched the binaries.  Linux and macOS use no prebuilt tree: they build a
# minimal static FFmpeg from source, the same one as MioPan and ReiPan.  It is
# --disable-everything plus the MPEG-2 decoder and its parser, so a new FFmpeg call
# in libmpeg_video.cpp may need its component enabled here.

set(ROSEPAN_FFMPEG_LIBS avcodec avutil swscale)
set(ROSEPAN_HAVE_FFMPEG OFF)

if(NOT WIN32 AND NOT ANDROID)
    enable_language(C)
    include(FetchContent)
    include(ExternalProject)
    include(ProcessorCount)
    FetchContent_Declare(ffmpeg
            GIT_REPOSITORY https://github.com/FFmpeg/FFmpeg.git
            GIT_TAG n8.1
            GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(ffmpeg)

    find_program(BASH_EXECUTABLE bash REQUIRED)
    find_program(MAKE_EXECUTABLE make REQUIRED)
    find_package(Threads REQUIRED)
    ProcessorCount(FFMPEG_BUILD_JOBS)
    if(NOT FFMPEG_BUILD_JOBS)
        set(FFMPEG_BUILD_JOBS 1)
    endif()

    set(FFMPEG_PREFIX "${CMAKE_BINARY_DIR}/ffmpeg")
    set(FFMPEG_INCLUDE_DIR "${FFMPEG_PREFIX}/include")
    file(MAKE_DIRECTORY "${FFMPEG_INCLUDE_DIR}")
    ExternalProject_Add(ffmpeg_build
            SOURCE_DIR "${ffmpeg_SOURCE_DIR}"
            CONFIGURE_COMMAND ${BASH_EXECUTABLE} <SOURCE_DIR>/configure
                --prefix=${FFMPEG_PREFIX}
                --cc=${CMAKE_C_COMPILER}
                --disable-shared --enable-static --enable-pic
                --disable-programs --disable-doc --disable-debug
                --disable-autodetect --disable-x86asm --disable-everything
                --disable-avdevice --disable-avformat --disable-avfilter
                --disable-swresample
                --enable-avcodec --enable-avutil --enable-swscale
                --enable-decoder=mpeg2video --enable-parser=mpegvideo
            BUILD_COMMAND ${MAKE_EXECUTABLE} -j${FFMPEG_BUILD_JOBS}
            INSTALL_COMMAND ${MAKE_EXECUTABLE} install
            BUILD_BYPRODUCTS
                "${FFMPEG_PREFIX}/lib/libavcodec.a"
                "${FFMPEG_PREFIX}/lib/libavutil.a"
                "${FFMPEG_PREFIX}/lib/libswscale.a")

    foreach(ffmpeg_lib IN LISTS ROSEPAN_FFMPEG_LIBS)
        add_library(${ffmpeg_lib} STATIC IMPORTED)
        set_target_properties(${ffmpeg_lib} PROPERTIES
                IMPORTED_LOCATION "${FFMPEG_PREFIX}/lib/lib${ffmpeg_lib}.a"
                INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIR}"
                INTERFACE_LINK_LIBRARIES "Threads::Threads;m;${CMAKE_DL_LIBS}")
        add_dependencies(${ffmpeg_lib} ffmpeg_build)
    endforeach()
    set_property(TARGET avcodec swscale APPEND PROPERTY INTERFACE_LINK_LIBRARIES avutil)
    set(ROSEPAN_HAVE_FFMPEG ON)
    message(STATUS "FFmpeg: building the MPEG-2 decoder from source -- video decoding enabled.")
    return()
endif()

# RosePan does not commit the 143 MB prebuilt tree; without one under extern/ it
# reuses the sibling MioPan checkout's (ROSEPAN_SIBLING_EXTERN, set in CMakeLists).
set(_rosepan_ffmpeg_default "${CMAKE_SOURCE_DIR}/extern/ffmpeg")
if(NOT EXISTS "${_rosepan_ffmpeg_default}/include/libavcodec/avcodec.h")
    set(_rosepan_ffmpeg_default "${ROSEPAN_SIBLING_EXTERN}/ffmpeg")
endif()
set(ROSEPAN_FFMPEG_ROOT "${_rosepan_ffmpeg_default}"
        CACHE PATH "Root of the prebuilt FFmpeg tree (include/, lib/, bin/).")

if(NOT EXISTS "${ROSEPAN_FFMPEG_ROOT}/include/libavcodec/avcodec.h")
    message(STATUS
            "FFmpeg not found at ${ROSEPAN_FFMPEG_ROOT} -- building without a "
            "video decoder.  Movies will play their audio over a black screen.")
    return()
endif()

set(FFMPEG_INCLUDE_DIR "${ROSEPAN_FFMPEG_ROOT}/include")

# Pick the import library flavour that matches the toolchain.  MinGW can
# usually link an MSVC .lib for a C DLL, but its own .dll.a is the path that
# always works, so prefer it when both are shipped.
function(_rosepan_ffmpeg_implib out_var lib)
    set(candidates)
    if(WIN32)
        if(MSVC)
            list(APPEND candidates
                    "${ROSEPAN_FFMPEG_ROOT}/lib/${lib}.lib"
                    "${ROSEPAN_FFMPEG_ROOT}/lib/lib${lib}.dll.a")
        else()
            list(APPEND candidates
                    "${ROSEPAN_FFMPEG_ROOT}/lib/lib${lib}.dll.a"
                    "${ROSEPAN_FFMPEG_ROOT}/lib/${lib}.lib")
        endif()
    else()
        list(APPEND candidates
                "${ROSEPAN_FFMPEG_ROOT}/lib/lib${lib}.so"
                "${ROSEPAN_FFMPEG_ROOT}/lib/lib${lib}.a")
    endif()

    foreach(candidate IN LISTS candidates)
        if(EXISTS "${candidate}")
            set(${out_var} "${candidate}" PARENT_SCOPE)
            return()
        endif()
    endforeach()

    set(${out_var} "" PARENT_SCOPE)
endfunction()

foreach(ffmpeg_lib IN LISTS ROSEPAN_FFMPEG_LIBS)
    _rosepan_ffmpeg_implib(ffmpeg_implib "${ffmpeg_lib}")
    if(NOT ffmpeg_implib)
        message(STATUS
                "FFmpeg tree at ${ROSEPAN_FFMPEG_ROOT} has headers but no import "
                "library for ${ffmpeg_lib} -- building without a video decoder.")
        return()
    endif()

    add_library(${ffmpeg_lib} SHARED IMPORTED)
    if(WIN32)
        set_target_properties(${ffmpeg_lib} PROPERTIES
                IMPORTED_IMPLIB "${ffmpeg_implib}"
                INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIR}")
    else()
        set_target_properties(${ffmpeg_lib} PROPERTIES
                IMPORTED_LOCATION "${ffmpeg_implib}"
                INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIR}")
    endif()
endforeach()

set(ROSEPAN_HAVE_FFMPEG ON)
message(STATUS "FFmpeg found at ${ROSEPAN_FFMPEG_ROOT} -- video decoding enabled.")
