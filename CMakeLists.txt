cmake_minimum_required(VERSION 3.28)
project(Helix.Platform LANGUAGES CXX)

if(NOT COMMAND helix_add_library)
    include("${CMAKE_CURRENT_SOURCE_DIR}/External/Helix.CMake/Helix.cmake")
endif()

helix_require(Core)
helix_add_library(helix_platform ALIAS Platform PUBLIC_DEPS Helix::Core)

if(HELIX_PLATFORM STREQUAL "windows")
    target_link_libraries(helix_platform PRIVATE user32)
elseif(HELIX_PLATFORM STREQUAL "linux")
    find_package(Threads REQUIRED)
    target_link_libraries(helix_platform PUBLIC Threads::Threads ${CMAKE_DL_LIBS})
elseif(HELIX_PLATFORM STREQUAL "android")
    target_link_libraries(helix_platform PUBLIC android log ${CMAKE_DL_LIBS})
endif()

# Applications on Android (the activity, its JNI glue, the APK) and the Web (the loader, the page).
include("${CMAKE_CURRENT_SOURCE_DIR}/Android/HelixAndroidApp.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/Web/HelixWebApp.cmake")

helix_add_test_suite(helix_platform_tests DEPS Helix::Platform)

if(PROJECT_IS_TOP_LEVEL)
    if(HELIX_PLATFORM STREQUAL "android")
        add_library(helix_window_demo SHARED Examples/WindowDemo/WindowDemo.cpp)
        helix_apply_options(helix_window_demo)
        set_target_properties(helix_window_demo PROPERTIES OUTPUT_NAME HelixWindowDemo)
        helix_add_android_app(helix_window_demo PACKAGE org.helix.windowdemo LABEL "Helix Window Demo")
    else()
        helix_add_executable(helix_window_demo SOURCES Examples/WindowDemo/WindowDemo.cpp DEPS Helix::Platform)
    endif()
    if(HELIX_PLATFORM STREQUAL "web")
        helix_add_web_app(helix_window_demo SHELL "${CMAKE_CURRENT_SOURCE_DIR}/Examples/Shared/WebShell.html")
    endif()
endif()
