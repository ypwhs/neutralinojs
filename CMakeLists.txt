cmake_minimum_required(VERSION 3.15)

# Set the project name and version
project(neutralino-win_x64 VERSION 5.3.0 LANGUAGES CXX)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set target output directory
set(OUTPUT_DIR "./bin")

# Include directories
include_directories(
    "."
    "src"
    "lib"
    "lib/asio/include"
    "lib/infoware/include"
    "lib/efsw/include"
    "lib/efsw/src"
)

if(WIN32)
    include_directories("lib/webview/windows")
endif()

# Source files
file(GLOB_RECURSE SOURCES
    "src/*.cpp"
    "auth/*.cpp"
    "server/*.cpp"
    "api/*/*.cpp"
    "lib/tinyprocess/process.cpp"
    "lib/easylogging/easylogging++.cc"
    "lib/platformfolders/platform_folders.cpp"
    "lib/clip/clip.cpp"
    "lib/clip/image.cpp"
    "lib/infoware/src/**/*.cpp"
    "lib/efsw/src/efsw/*.cpp"
)

if(UNIX AND NOT APPLE)
    file(GLOB_RECURSE LINUX_SOURCES
        "lib/tinyprocess/process_unix.cpp"
        "lib/clip/clip_x11.cpp"
        "lib/efsw/src/efsw/platform/posix/*.cpp"
    )
    list(APPEND SOURCES ${LINUX_SOURCES})
elseif(APPLE)
    file(GLOB_RECURSE DARWIN_SOURCES
        "lib/tinyprocess/process_unix.cpp"
        "lib/clip/clip_osx.mm"
        "lib/webview/macwindow.mm"
        "lib/efsw/src/efsw/platform/posix/*.cpp"
    )
    list(APPEND SOURCES ${DARWIN_SOURCES})
elseif(WIN32)
    file(GLOB_RECURSE WINDOWS_SOURCES
        "lib/tinyprocess/process_win.cpp"
        "lib/clip/clip_win.cpp"
        "lib/efsw/src/efsw/platform/win/*.cpp"
    )
    list(APPEND SOURCES ${WINDOWS_SOURCES})
endif()

# Compiler options
if(UNIX AND NOT APPLE)
    add_definitions(
        -DNEU_VERSION="${PROJECT_VERSION}"
        -DNEU_COMMIT="${GIT_COMMIT_HASH}"
        -DELPP_NO_DEFAULT_LOG_FILE
        -DASIO_STANDALONE
        -DINFOWARE_VERSION="0.6.0"
        -DINFOWARE_USE_X11
        -DCLIP_ENABLE_IMAGE
        -DHAVE_XCB_XLIB_H
        -DWEBVIEW_GTK=1
        -DTRAY_APPINDICATOR=1
        -DHAVE_PNG_H
    )
    # Add Linux specific options
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} $(pkg-config --cflags --libs gtk+-3.0 webkit2gtk-4.0 glib-2.0 xcb x11 xrandr) -pthread -lpng -lstdc++fs -ldl -no-pie -Os")
elseif(APPLE)
    add_definitions(
        -DNEU_VERSION="${PROJECT_VERSION}"
        -DNEU_COMMIT="${GIT_COMMIT_HASH}"
        -DELPP_NO_DEFAULT_LOG_FILE
        -DASIO_STANDALONE
        -DINFOWARE_VERSION="0.6.0"
        -DINFOWARE_USE_X11
        -DCLIP_ENABLE_IMAGE
        -DWEBVIEW_COCOA=1
        -DTRAY_APPKIT=1
        -DMACOSX_DEPLOYMENT_TARGET=10.7
    )
    # Add macOS specific options
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations -framework WebKit -framework Cocoa -Os")
elseif(WIN32)
    add_definitions(
        -DNEU_VERSION="${PROJECT_VERSION}"
        -DNEU_COMMIT="${GIT_COMMIT_HASH}"
        -DELPP_NO_DEFAULT_LOG_FILE
        -DASIO_STANDALONE
        -DINFOWARE_VERSION="0.6.0"
        -DINFOWARE_USE_X11
        -DCLIP_ENABLE_IMAGE
        -D_WEBSOCKETPP_CPP11_STL_
        -D_HAS_STD_BYTE=0
        -DTRAY_WINAPI=1
        -DUNICODE
        -D_UNICODE
    )
    # Add Windows specific options
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8 /EHsc /Os")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Fobin/")
endif()

# Define the executable target
add_executable(${PROJECT_NAME} WIN32 ${SOURCES})

# Post-build step for copying to bin directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
)

if(WIN32)
    # Link libraries for Windows
    set(WEBVIEW2LOADER_STATIC_LIB "${CMAKE_SOURCE_DIR}/lib/webview/windows/WebView2LoaderStatic.lib")
    target_link_libraries(${PROJECT_NAME}
        ${WEBVIEW2LOADER_STATIC_LIB}
        "gdi32.lib"
        "version.lib"
        "Ole32.lib"
        "OleAut32.lib"
        "wbemuuid.lib"
        "ntdll.lib"
        "dwmapi.lib"
    )
endif()
