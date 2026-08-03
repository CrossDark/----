cmake_minimum_required(VERSION 3.16)

# =====================================================================
# 世界生成器 - 统一构建脚本
# 通过 CMake 选项切换 CLI / GUI 两种产物,核心算法代码共用一套
#
# 用法:
#   CLI 版 (默认,仅依赖 zlib):
#       cmake -S . -B build-cli -DCMAKE_BUILD_TYPE=Release
#       cmake --build build-cli
#
#   GUI 版 (依赖 Qt6 Widgets + Concurrent):
#       cmake -S . -B build-gui -DBUILD_GUI=ON -DCMAKE_BUILD_TYPE=Release
#       cmake --build build-gui
# =====================================================================

option(BUILD_GUI "构建带 Qt6 界面的 GUI 版本 (OFF=命令行版)" OFF)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(BUILD_GUI)
    project(worldgen_gui LANGUAGES C CXX)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_AUTOMOC ON)
else()
    project(worldgen LANGUAGES C)
endif()

# ---------- 核心算法 (两套产物共用) ----------
set(WORLDGEN_CORE_SOURCES
    src/core/worldgen_core.c
    src/core/worldgen_core.h
)

# zlib (CLI/GUI 都需要,用于输出 PNG)
find_package(ZLIB REQUIRED)

# ---------- 目标产物 ----------
if(BUILD_GUI)
    # ===== GUI 版: Qt6 应用 =====
    find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent)
    qt_standard_project_setup()

    # Qt 资源 (QSS 样式表 + 窗口图标) 编译进二进制,避免运行时依赖工作目录
    qt_add_resources(WORLDGEN_QRC src/gui/resources.qrc)

    add_executable(worldgen_gui
        src/gui/main.cpp
        src/gui/mainwindow.cpp
        src/gui/mainwindow.h
        ${WORLDGEN_CORE_SOURCES}
        ${WORLDGEN_QRC}
    )

    target_include_directories(worldgen_gui PRIVATE src/core src/gui)

    # macOS: 打包为 .app 应用包
    set_target_properties(worldgen_gui PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.crossdark.worldgen"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/Info.plist"
        MACOSX_BUNDLE_BUNDLE_NAME "世界生成器"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
    )

    # Windows: 无控制台窗口的 GUI 程序
    if(WIN32)
        set_target_properties(worldgen_gui PROPERTIES WIN32_EXECUTABLE TRUE)
    endif()

    target_link_libraries(worldgen_gui PRIVATE
        Qt6::Widgets
        Qt6::Concurrent
        ZLIB::ZLIB
    )
    if(UNIX)
        target_link_libraries(worldgen_gui PRIVATE m)
    endif()

else()
    # ===== CLI 版: 纯 C 命令行程序 =====
    add_executable(worldgen
        src/cli/main.c
        ${WORLDGEN_CORE_SOURCES}
    )

    target_include_directories(worldgen PRIVATE src/core src/cli)
    target_link_libraries(worldgen PRIVATE ZLIB::ZLIB)
    if(UNIX)
        target_link_libraries(worldgen PRIVATE m)
    endif()
endif()
