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

helix_add_test_suite(helix_platform_tests DEPS Helix::Platform)

if(PROJECT_IS_TOP_LEVEL)
    helix_add_executable(helix_window_demo SOURCES Examples/WindowDemo/WindowDemo.cpp DEPS Helix::Platform)
    if(HELIX_PLATFORM STREQUAL "web")
        set_target_properties(helix_window_demo PROPERTIES SUFFIX ".html")
        target_link_options(helix_window_demo PRIVATE -sENVIRONMENT=web --shell-file "${CMAKE_CURRENT_SOURCE_DIR}/Examples/Shared/WebShell.html")
    endif()
endif()
