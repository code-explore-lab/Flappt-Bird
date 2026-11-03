# =============================================================================
#  CMakeLists.txt —— Flappy Bird（C++17 + SDL2 + SDL2_ttf）
#
#  目录结构：
#    src/main.cpp          入口
#    src/game.{h,cpp}      主循环、状态机、HUD
#    src/bird.{h,cpp}      小鸟
#    src/pipe.{h,cpp}      管道与对象池
#    src/text.{h,cpp}      文字纹理缓存
#    src/{config,state,raii,util}.h  常量 / 枚举 / RAII / 工具
#
#  三平台通用：
#    Linux/macOS :  cmake -S . -B build && cmake --build build
#    Windows/MSVC :  cmake -S . -B build -DSDL2_DIR="C:/SDL2/lib/cmake/SDL2" ...
#    Windows/MinGW:  cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="C:/SDL2/x86_64-w64-mingw32;C:/SDL2_ttf/x86_64-w64-mingw32"
#
#  可选开关（见 README）：
#    -DFLAPPY_USE_SDL2_IMAGE=ON     预埋 SDL2_image（贴图版）
#    -DFLAPPY_USE_SDL2_MIXER=ON     预埋 SDL2_mixer（音效版）
#    -DFLAPPY_SDL_MAIN_HANDLED=ON   不使用 SDL2main，自己接管入口
#    -DFLAPPY_WINDOWS_GUI=ON        Windows 下隐藏控制台（需 SDL2main）
#    -DFLAPPY_COPY_RUNTIME=OFF      不自动复制 assets/ 和 DLL
# =============================================================================
cmake_minimum_required(VERSION 3.16)
project(flappy_bird VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 单配置生成器（Makefiles / Ninja）默认用 Release；VS 等多配置生成器不受影响
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "构建类型（Debug/Release/RelWithDebInfo）" FORCE)
endif()

option(FLAPPY_USE_SDL2_IMAGE   "启用 SDL2_image（为贴图版预留）" OFF)
option(FLAPPY_USE_SDL2_MIXER   "启用 SDL2_mixer（为音效版预留）" OFF)
option(FLAPPY_SDL_MAIN_HANDLED "定义 FLAPPY_SDL_MAIN_HANDLED：源码里自行 SDL_SetMainReady()，不链接 SDL2main" OFF)
option(FLAPPY_WINDOWS_GUI      "Windows：使用 GUI 子系统（不弹控制台窗口）" OFF)
option(FLAPPY_COPY_RUNTIME     "构建后复制 assets/（Windows 还复制 DLL）到可执行文件目录" ON)

if (WIN32 AND FLAPPY_WINDOWS_GUI AND FLAPPY_SDL_MAIN_HANDLED)
    message(FATAL_ERROR
        "FLAPPY_WINDOWS_GUI=ON 与 FLAPPY_SDL_MAIN_HANDLED=ON 不能同时使用：\n"
        "GUI 子系统需要 SDL2main 提供 WinMain 入口；若手动接管入口，请保留控制台子系统。")
endif()

# -----------------------------------------------------------------------------
#  1. 目标：核心静态库（方便写测试 / 复用）+ 可执行文件
# -----------------------------------------------------------------------------
add_library(flappy_core STATIC
    src/bird.cpp
    src/pipe.cpp
    src/text.cpp
    src/game.cpp)

target_include_directories(flappy_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_executable(flappy src/main.cpp)

# -----------------------------------------------------------------------------
#  2. 查找 SDL2：优先官方 CMake 配置包（SDL2::SDL2），退回到 pkg-config
# -----------------------------------------------------------------------------
find_package(PkgConfig QUIET)

set(FLAPPY_SDL2_MAIN_LIBS "")
set(FLAPPY_SDL2_LIBS "")

find_package(SDL2 CONFIG QUIET)

if (TARGET SDL2::SDL2)
    list(APPEND FLAPPY_SDL2_LIBS SDL2::SDL2)
elseif (PkgConfig_FOUND)
    pkg_check_modules(SDL2_PC QUIET IMPORTED_TARGET sdl2)
    if (TARGET PkgConfig::SDL2_PC)
        list(APPEND FLAPPY_SDL2_LIBS PkgConfig::SDL2_PC)
    endif()
endif()

if (NOT FLAPPY_SDL2_LIBS)
    message(FATAL_ERROR
        "未找到 SDL2。\n"
        "  Ubuntu/Debian : sudo apt install libsdl2-dev\n"
        "  macOS         : brew install sdl2\n"
        "  Windows/MinGW : 解压 SDL2-devel-*-mingw.zip，然后\n"
        "                  cmake -B build -DCMAKE_PREFIX_PATH=\"C:/SDL2/x86_64-w64-mingw32\"\n"
        "  或指定 SDL2_DIR 指向 .../lib/cmake/SDL2")
endif()

# Windows 上默认链接 SDL2main（它提供 WinMain 转发到 main）
if (WIN32 AND NOT FLAPPY_SDL_MAIN_HANDLED AND TARGET SDL2::SDL2main)
    list(APPEND FLAPPY_SDL2_MAIN_LIBS SDL2::SDL2main)
elseif (WIN32 AND NOT FLAPPY_SDL_MAIN_HANDLED AND NOT TARGET SDL2::SDL2main)
    message(STATUS "未发现 SDL2::SDL2main，改用控制台子系统直接入口（如遇 WinMain 缺失请开启 FLAPPY_SDL_MAIN_HANDLED）")
endif()

# -----------------------------------------------------------------------------
#  3. 查找 SDL2_ttf：配置包 → pkg-config → 老式变量
# -----------------------------------------------------------------------------
set(FLAPPY_TTF_LIBS "")

find_package(SDL2_ttf CONFIG QUIET)

if (TARGET SDL2_ttf::SDL2_ttf)
    list(APPEND FLAPPY_TTF_LIBS SDL2_ttf::SDL2_ttf)
elseif (TARGET SDL2_ttf::SDL2_ttf-static)
    list(APPEND FLAPPY_TTF_LIBS SDL2_ttf::SDL2_ttf-static)
elseif (PkgConfig_FOUND)
    pkg_check_modules(SDL2_TTF_PC QUIET IMPORTED_TARGET SDL2_ttf)
    if (TARGET PkgConfig::SDL2_TTF_PC)
        list(APPEND FLAPPY_TTF_LIBS PkgConfig::SDL2_TTF_PC)
    endif()
endif()

if (NOT FLAPPY_TTF_LIBS AND SDL2_ttf_FOUND AND SDL2_ttf_LIBRARIES)
    # 部分发行版只提供 FindSDL2_ttf 的变量形式
    target_include_directories(flappy_core PRIVATE ${SDL2_ttf_INCLUDE_DIRS})
    list(APPEND FLAPPY_TTF_LIBS ${SDL2_ttf_LIBRARIES})
endif()

if (NOT FLAPPY_TTF_LIBS)
    message(FATAL_ERROR
        "未找到 SDL2_ttf。\n"
        "  Ubuntu/Debian : sudo apt install libsdl2-ttf-dev\n"
        "  macOS         : brew install sdl2_ttf\n"
        "  Windows/MinGW : 解压 SDL2_ttf-devel-*-mingw.zip，并加入 CMAKE_PREFIX_PATH")
endif()

target_link_libraries(flappy_core PUBLIC ${FLAPPY_SDL2_LIBS} ${FLAPPY_TTF_LIBS})
target_link_libraries(flappy PRIVATE ${FLAPPY_SDL2_MAIN_LIBS} flappy_core)

# -----------------------------------------------------------------------------
#  4. 可选扩展库（源码默认不调用，打开后只是把库链接进来，方便二次开发）
# -----------------------------------------------------------------------------
if (FLAPPY_USE_SDL2_IMAGE)
    set(FLAPPY_SDL2_IMAGE_FOUND OFF)
    find_package(SDL2_image CONFIG QUIET)
    if (TARGET SDL2_image::SDL2_image)
        target_link_libraries(flappy_core PUBLIC SDL2_image::SDL2_image)
        set(FLAPPY_SDL2_IMAGE_FOUND ON)
    elseif (PkgConfig_FOUND)
        pkg_check_modules(SDL2_IMAGE_PC QUIET IMPORTED_TARGET SDL2_image)
        if (TARGET PkgConfig::SDL2_IMAGE_PC)
            target_link_libraries(flappy_core PUBLIC PkgConfig::SDL2_IMAGE_PC)
            set(FLAPPY_SDL2_IMAGE_FOUND ON)
        endif()
    endif()
    if (NOT FLAPPY_SDL2_IMAGE_FOUND)
        message(FATAL_ERROR "开启了 FLAPPY_USE_SDL2_IMAGE 但没找到 SDL2_image，请先安装（如 libsdl2-image-dev / brew install sdl2_image）。")
    endif()
    target_compile_definitions(flappy_core PRIVATE FLAPPY_HAVE_SDL2_IMAGE=1)
    message(STATUS "SDL2_image 已开启（贴图版）")
endif()

if (FLAPPY_USE_SDL2_MIXER)
    set(FLAPPY_SDL2_MIXER_FOUND OFF)
    find_package(SDL2_mixer CONFIG QUIET)
    if (TARGET SDL2_mixer::SDL2_mixer)
        target_link_libraries(flappy_core PUBLIC SDL2_mixer::SDL2_mixer)
        set(FLAPPY_SDL2_MIXER_FOUND ON)
    elseif (PkgConfig_FOUND)
        pkg_check_modules(SDL2_MIXER_PC QUIET IMPORTED_TARGET SDL2_mixer)
        if (TARGET PkgConfig::SDL2_MIXER_PC)
            target_link_libraries(flappy_core PUBLIC PkgConfig::SDL2_MIXER_PC)
            set(FLAPPY_SDL2_MIXER_FOUND ON)
        endif()
    endif()
    if (NOT FLAPPY_SDL2_MIXER_FOUND)
        message(FATAL_ERROR "开启了 FLAPPY_USE_SDL2_MIXER 但没找到 SDL2_mixer，请先安装（如 libsdl2-mixer-dev / brew install sdl2_mixer）。")
    endif()
    target_compile_definitions(flappy_core PRIVATE FLAPPY_HAVE_SDL2_MIXER=1)
    message(STATUS "SDL2_mixer 已开启（音效版）")
endif()

# -----------------------------------------------------------------------------
#  5. 编译选项 / 入口方式
# -----------------------------------------------------------------------------
if (FLAPPY_SDL_MAIN_HANDLED)
    target_compile_definitions(flappy PRIVATE FLAPPY_SDL_MAIN_HANDLED=1)
endif()

if (MSVC)
    # /utf-8：源码含中文注释与中文字符串，必须让 MSVC 按 UTF-8 解析
    foreach (target_name flappy_core flappy)
        target_compile_options(${target_name} PRIVATE /utf-8 /W4 /permissive-)
    endforeach()
else()
    foreach (target_name flappy_core flappy)
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
    endforeach()
endif()

if (MINGW)
    # 让 exe 不依赖 libstdc++-6.dll / libgcc_s_seh-1.dll，方便拷给别人
    target_link_options(flappy PRIVATE -static-libgcc -static-libstdc++)
endif()

if (WIN32 AND FLAPPY_WINDOWS_GUI)
    set_target_properties(flappy PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# -----------------------------------------------------------------------------
#  6. 构建后复制资源：assets/（字体、以及将来的贴图/音效）
# -----------------------------------------------------------------------------
if (FLAPPY_COPY_RUNTIME)
    add_custom_command(TARGET flappy POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/assets"
                "$<TARGET_FILE_DIR:flappy>/assets"
        COMMENT "复制 assets/ 到可执行文件目录")
endif()

# -----------------------------------------------------------------------------
#  7. Windows 下尝试自动复制 SDL2.dll / SDL2_ttf.dll
#     找不到时只提示，不影响构建；README 里有手动复制说明。
# -----------------------------------------------------------------------------
if (WIN32 AND FLAPPY_COPY_RUNTIME)
    function(flappy_copy_sdl_dll target_name)
        if (NOT TARGET ${target_name})
            return()
        endif()
        # 只处理动态库目标；静态库/接口目标跳过
        get_target_property(target_type ${target_name} TYPE)
        if (NOT target_type STREQUAL "SHARED_LIBRARY" AND NOT target_type STREQUAL "MODULE_LIBRARY")
            return()
        endif()
        # $<TARGET_FILE:...> 会自动取对应当前配置的 DLL 路径
        add_custom_command(TARGET flappy POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<TARGET_FILE:${target_name}>" "$<TARGET_FILE_DIR:flappy>"
            COMMENT "复制 $<TARGET_FILE_NAME:${target_name}>")
    endfunction()

    flappy_copy_sdl_dll(SDL2::SDL2)
    flappy_copy_sdl_dll(SDL2_ttf::SDL2_ttf)
endif()

# -----------------------------------------------------------------------------
#  8. 安装（可选）：cmake --install build --prefix dist
# -----------------------------------------------------------------------------
install(TARGETS flappy RUNTIME DESTINATION .)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets" DESTINATION .)
