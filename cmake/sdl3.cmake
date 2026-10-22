include(FetchContent)

FetchContent_Declare(
        sdl3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.4.12
)
if(ANDROID)
    set(SDL_STATIC OFF)
    set(SDL_SHARED ON)
    set(SDL_ANDROID_JAR ON)
else()
    set(SDL_STATIC ON)
    set(SDL_SHARED OFF)
endif()
FetchContent_MakeAvailable(sdl3)

foreach(sdl_target SDL3-static SDL3-shared SDL3_test)
    if(TARGET ${sdl_target})
        set_target_properties(${sdl_target} PROPERTIES
                DISABLE_PRECOMPILE_HEADERS ON)
    endif()
endforeach()

if(ANDROID)
    set(ROSEPAN_SDL_ACTIVITY_JAVA
            "${sdl3_SOURCE_DIR}/android-project/app/src/main/java/org/libsdl/app/SDLActivity.java")
    if(EXISTS "${ROSEPAN_SDL_ACTIVITY_JAVA}")
        file(READ "${ROSEPAN_SDL_ACTIVITY_JAVA}" ROSEPAN_SDL_ACTIVITY_CONTENT)
        set(ROSEPAN_SDL_ACTIVITY_OPEN_FD_OLD [=[
        } catch (FileNotFoundException e) {
            e.printStackTrace();
            return -1;
        }
]=])
        set(ROSEPAN_SDL_ACTIVITY_OPEN_FD_NEW [=[
        } catch (Exception e) {
            e.printStackTrace();
            return -1;
        }
]=])
        if(ROSEPAN_SDL_ACTIVITY_CONTENT MATCHES "catch \\(FileNotFoundException e\\)")
            string(REPLACE "${ROSEPAN_SDL_ACTIVITY_OPEN_FD_OLD}"
                    "${ROSEPAN_SDL_ACTIVITY_OPEN_FD_NEW}"
                    ROSEPAN_SDL_ACTIVITY_CONTENT
                    "${ROSEPAN_SDL_ACTIVITY_CONTENT}")
            file(WRITE "${ROSEPAN_SDL_ACTIVITY_JAVA}" "${ROSEPAN_SDL_ACTIVITY_CONTENT}")
        endif()
    endif()

    set(ROSEPAN_SDL_ANDROID_C
            "${sdl3_SOURCE_DIR}/src/core/android/SDL_android.c")
    if(EXISTS "${ROSEPAN_SDL_ANDROID_C}")
        file(READ "${ROSEPAN_SDL_ANDROID_C}" ROSEPAN_SDL_ANDROID_CONTENT)
        set(ROSEPAN_SDL_OPEN_FD_JNI_OLD [=[
    jint fd = (*env)->CallStaticIntMethod(env, mActivityClass, midOpenFileDescriptor, jstringUri, jstringMode);
    (*env)->DeleteLocalRef(env, jstringUri);
]=])
        set(ROSEPAN_SDL_OPEN_FD_JNI_NEW [=[
    jint fd = (*env)->CallStaticIntMethod(env, mActivityClass, midOpenFileDescriptor, jstringUri, jstringMode);
    if ((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionClear(env);
        fd = -1;
    }
    (*env)->DeleteLocalRef(env, jstringUri);
]=])
        string(FIND "${ROSEPAN_SDL_ANDROID_CONTENT}"
                "${ROSEPAN_SDL_OPEN_FD_JNI_NEW}"
                ROSEPAN_SDL_OPEN_FD_JNI_ALREADY_PATCHED)
        if(ROSEPAN_SDL_OPEN_FD_JNI_ALREADY_PATCHED EQUAL -1)
            string(REPLACE "${ROSEPAN_SDL_OPEN_FD_JNI_OLD}"
                    "${ROSEPAN_SDL_OPEN_FD_JNI_NEW}"
                    ROSEPAN_SDL_ANDROID_CONTENT
                    "${ROSEPAN_SDL_ANDROID_CONTENT}")
            file(WRITE "${ROSEPAN_SDL_ANDROID_C}" "${ROSEPAN_SDL_ANDROID_CONTENT}")
        endif()
    endif()
endif()
