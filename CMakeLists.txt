cmake_minimum_required (VERSION 3.20)

if (APPLE)
    set (LANGUAGE_APPLE ObjC)
endif ()

project (NativeFileDialogDemo
    VERSION 0.1.0
    LANGUAGES C CXX ${LANGUAGE_APPLE}
)

add_subdirectory (deps/dpf)

include_directories (
    plugin/
    deps/dpf-widgets/opengl/
    deps/nativefiledialog/src/include/
)

if (WIN32)
    set (SRC_NFD_PLATFORM deps/nativefiledialog/src/nfd_win.cpp)
elseif (UNIX AND NOT APPLE)
    set (SRC_NFD_PLATFORM deps/nativefiledialog/src/nfd_gtk.c)

    find_package(PkgConfig)
    pkg_check_modules(GTK "gtk+-3.0" REQUIRED)

    include_directories (${GTK_INCLUDE_DIRS})
elseif (APPLE)
    set (SRC_NFD_PLATFORM deps/nativefiledialog/src/nfd_cocoa.m)
else ()
    message (FATAL_ERROR "Unsupported platform detected. Cannot use NFD.")
endif ()


add_library (nativefiledialog STATIC
    deps/nativefiledialog/src/nfd_common.c
    ${SRC_NFD_PLATFORM}
)

dpf_add_plugin (${PROJECT_NAME}
    TARGETS vst2 vst3 lv2 clap jack
    FILES_DSP
        plugin/NativeFileDialogDemo.cpp
    FILES_UI
        plugin/NativeFileDialogDemo_UI.cpp
        deps/dpf-widgets/opengl/DearImGui.cpp
)

if (UNIX AND NOT APPLE)
    target_link_libraries (${PROJECT_NAME}-ui PRIVATE ${GTK_LIBRARIES})
endif ()

target_link_libraries (${PROJECT_NAME} PUBLIC nativefiledialog)
