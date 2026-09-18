cmake_minimum_required(VERSION 3.20)
project(AetherWave VERSION 1.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

# Common cross-platform core files
set(CORE_SOURCES
    src/dsp.cpp
    src/color_mixer.cpp
    src/config_manager.cpp
)

if(WIN32)
    add_compile_definitions(NOMINMAX)
    set(PLATFORM_SOURCES
        src/main.cpp
        src/audio_capture.cpp
        src/renderer.cpp
        src/window.cpp
    )
    add_executable(AetherWave WIN32 ${CORE_SOURCES} ${PLATFORM_SOURCES})
    target_link_libraries(AetherWave PRIVATE
        user32
        gdi32
        shell32
        ole32
        d2d1
    )

elseif(APPLE)
    enable_language(OBJCXX)
    set(PLATFORM_SOURCES
        src/platform/macos/audio_capture_macos.mm
        src/platform/macos/window_macos.mm
        src/main.cpp
    )
    add_executable(AetherWave ${CORE_SOURCES} ${PLATFORM_SOURCES})
    target_link_libraries(AetherWave PRIVATE
        "-framework Cocoa"
        "-framework AudioToolbox"
        "-framework CoreAudio"
        "-framework CoreGraphics"
    )

elseif(UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PULSE REQUIRED libpulse libpulse-simple)
    pkg_check_modules(CAIRO REQUIRED cairo)
    find_package(X11 REQUIRED)

    include_directories(
        ${PULSE_INCLUDE_DIRS}
        ${CAIRO_INCLUDE_DIRS}
        ${X11_INCLUDE_DIR}
    )

    set(PLATFORM_SOURCES
        src/platform/linux/audio_capture_pulse.cpp
        src/platform/linux/window_linux.cpp
        src/main.cpp
    )
    add_executable(AetherWave ${CORE_SOURCES} ${PLATFORM_SOURCES})
    target_link_libraries(AetherWave PRIVATE
        ${PULSE_LIBRARIES}
        ${CAIRO_LIBRARIES}
        ${X11_LIBRARIES}
        Xext
        pthread
    )
endif()
