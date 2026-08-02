cmake_minimum_required(VERSION 3.16)
project(worldgen_gui LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent)

qt_standard_project_setup()

add_executable(worldgen_gui
    main.cpp
    mainwindow.cpp
    mainwindow.h
    worldgen_core.c
    worldgen_core.h
)

# macOS: 打包为 .app 应用包
set_target_properties(worldgen_gui PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.crossdark.worldgen"
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
    MACOSX_BUNDLE_BUNDLE_NAME "世界生成器"
    MACOSX_BUNDLE_BUNDLE_VERSION "1.0"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
)

# worldgen_core.c 需要 zlib 以写 PNG
find_library(ZLIB_LIB z)
target_link_libraries(worldgen_gui PRIVATE Qt6::Widgets Qt6::Concurrent ${ZLIB_LIB} m)
