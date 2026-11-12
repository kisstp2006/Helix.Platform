# helix_add_android_app(<shared library target> [PACKAGE <name>] [LABEL <text>] [ASSETS <folder>] [ASSETS_PATH <path>]
#                       [PLATFORM <library target>] [LIBRARIES <shared library>...])
#
# Makes the shared library an Android application: the JNI glue of org.helix.platform.HelixActivity is linked into it
# (the application defines HelixMain) with the platform library (PLATFORM, Helix::Platform by default; a shared library
# holding it, such as the shared engine, for applications of several libraries), and when the SDK's build tools and
# platform and a JDK are found, <target>_apk builds <output name>.apk: the activity, the manifest, the library, the
# shared libraries it loads (LIBRARIES: targets or files) with the shared C++ runtime, and the files of ASSETS under the
# assets' ASSETS_PATH, signed with a debug key made in the build folder.

set(HELIX_PLATFORM_ANDROID_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE INTERNAL "")

function(helix_add_android_app target)
    cmake_parse_arguments(ARG "" "PACKAGE;LABEL;ASSETS;ASSETS_PATH;PLATFORM" "LIBRARIES" ${ARGN})
    set(dir "${HELIX_PLATFORM_ANDROID_DIR}")
    if(NOT ARG_PLATFORM)
        set(ARG_PLATFORM Helix::Platform)
    endif()
    target_sources(${target} PRIVATE "${dir}/Native/HelixActivityGlue.cpp")
    target_link_libraries(${target} PRIVATE ${ARG_PLATFORM} android log)
    set(libraries "")
    set(library_targets "")
    foreach(l IN LISTS ARG_LIBRARIES)
        if(TARGET ${l})
            list(APPEND libraries "$<TARGET_FILE:${l}>")
            list(APPEND library_targets ${l})
        else()
            list(APPEND libraries "${l}")
        endif()
    endforeach()
    if(HELIX_ANDROID_STL_LIBRARY)
        list(APPEND libraries "${HELIX_ANDROID_STL_LIBRARY}")
    endif()
    if(NOT ARG_PACKAGE)
        set(ARG_PACKAGE "org.helix.${target}")
    endif()
    if(NOT ARG_LABEL)
        set(ARG_LABEL "${target}")
    endif()
    get_target_property(library ${target} OUTPUT_NAME)
    if(NOT library)
        set(library "${target}")
    endif()

    # The SDK's newest build tools and platform, and a JDK.
    set(sdk "$ENV{ANDROID_HOME}")
    if(NOT sdk)
        set(sdk "$ENV{ANDROID_SDK_ROOT}")
    endif()
    file(GLOB build_tools_dirs "${sdk}/build-tools/*")
    list(SORT build_tools_dirs COMPARE NATURAL ORDER DESCENDING)
    file(GLOB platform_jars "${sdk}/platforms/android-*/android.jar")
    list(SORT platform_jars COMPARE NATURAL ORDER DESCENDING)
    find_program(HELIX_JAVAC NAMES javac HINTS "$ENV{JAVA_HOME}/bin")
    find_program(HELIX_JAR NAMES jar HINTS "$ENV{JAVA_HOME}/bin")
    find_program(HELIX_KEYTOOL NAMES keytool HINTS "$ENV{JAVA_HOME}/bin")
    if(NOT build_tools_dirs OR NOT platform_jars OR NOT HELIX_JAVAC OR NOT HELIX_JAR OR NOT HELIX_KEYTOOL)
        message(STATUS "Helix: no Android SDK build tools, platform or JDK found; ${library}.apk is not built (lib${library}.so is).")
        return()
    endif()
    list(GET build_tools_dirs 0 build_tools)
    list(GET platform_jars 0 android_jar)
    get_filename_component(platform_dir "${android_jar}" DIRECTORY)
    get_filename_component(platform_name "${platform_dir}" NAME)
    string(REPLACE "android-" "" HELIX_ANDROID_TARGET_SDK "${platform_name}")
    set(HELIX_ANDROID_MIN_SDK "${HELIX_ANDROID_API}")
    set(HELIX_ANDROID_PACKAGE "${ARG_PACKAGE}")
    set(HELIX_ANDROID_LABEL "${ARG_LABEL}")
    set(HELIX_ANDROID_LIBRARY "${library}")
    set(work "${CMAKE_CURRENT_BINARY_DIR}/${target}.Apk")
    configure_file("${dir}/Templates/AndroidManifest.xml" "${work}/AndroidManifest.xml" @ONLY)
    file(GLOB_RECURSE java_sources CONFIGURE_DEPENDS "${dir}/HelixActivity/src/main/java/*.java")
    set(apk "${CMAKE_CURRENT_BINARY_DIR}/${library}.apk")
    add_custom_command(
        OUTPUT "${apk}"
        COMMAND "${CMAKE_COMMAND}" "-DBUILD_TOOLS=${build_tools}" "-DANDROID_JAR=${android_jar}" "-DJAVAC=${HELIX_JAVAC}" "-DJAR=${HELIX_JAR}"
                "-DKEYTOOL=${HELIX_KEYTOOL}" "-DMANIFEST=${work}/AndroidManifest.xml" "-DJAVA_SOURCES=$<JOIN:${java_sources},|>"
                "-DLIBRARY=$<TARGET_FILE:${target}>" "-DLIBRARIES=$<JOIN:${libraries},|>" "-DABI=${HELIX_ANDROID_ABI}" "-DMIN_SDK=${HELIX_ANDROID_MIN_SDK}"
                "-DASSETS=${ARG_ASSETS}" "-DASSETS_PATH=${ARG_ASSETS_PATH}" "-DWORK=${work}" "-DOUTPUT=${apk}" -P "${dir}/BuildApk.cmake"
        DEPENDS ${target} ${library_targets} ${java_sources} "${work}/AndroidManifest.xml" "${dir}/BuildApk.cmake"
        COMMENT "Packaging ${library}.apk")
    add_custom_target(${target}_apk ALL DEPENDS "${apk}")
endfunction()
