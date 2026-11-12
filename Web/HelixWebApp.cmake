# helix_add_web_app(<executable target> SHELL <html template> [FILES <folder>] [FILES_PATH <path>])
#
# Makes the executable a Web application: <output name>.html from the template, the loader of Helix.Platform (it
# fetches the files the page's Module.helixPreload names before main, and takes ?args=), the file system. The files of
# FILES are copied next to the page under FILES_PATH.

set(HELIX_PLATFORM_WEB_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE INTERNAL "")

function(helix_add_web_app target)
    cmake_parse_arguments(ARG "" "SHELL;FILES;FILES_PATH" "" ${ARGN})
    set(loader "${HELIX_PLATFORM_WEB_DIR}/Loader/HelixPreload.js")
    set_target_properties(${target} PROPERTIES SUFFIX ".html")
    target_link_options(${target} PRIVATE -sENVIRONMENT=web -sEXIT_RUNTIME=0 -sFORCE_FILESYSTEM=1 "SHELL:--shell-file \"${ARG_SHELL}\""
                        "SHELL:--pre-js \"${loader}\"")
    set_property(TARGET ${target} APPEND PROPERTY LINK_DEPENDS "${ARG_SHELL}" "${loader}")
    if(ARG_FILES)
        add_custom_command(TARGET ${target} POST_BUILD
                           COMMAND "${CMAKE_COMMAND}" -E copy_directory "${ARG_FILES}" "$<TARGET_FILE_DIR:${target}>/${ARG_FILES_PATH}")
    endif()
endfunction()
