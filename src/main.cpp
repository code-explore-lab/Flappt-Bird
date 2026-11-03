// =============================================================================
//  main.cpp —— 程序入口
//
//  Windows 入口说明（按需二选一）：
//    默认：普通 main()，由 SDL2main 提供 WinMain 转发（CMake 会自动链接）。
//    备选：定义 FLAPPY_SDL_MAIN_HANDLED（CMake 选项 FLAPPY_SDL_MAIN_HANDLED=ON），
//          则屏蔽 SDL 的 main 宏，并在这里手动调用 SDL_SetMainReady()。
//  两种方式都不会混入任何平台专属 API，源码保持跨平台。
// =============================================================================
#ifdef FLAPPY_SDL_MAIN_HANDLED
#  ifndef SDL_MAIN_HANDLED
#    define SDL_MAIN_HANDLED
#  endif
#endif

#include <cstdio>
#include <exception>

#include "game.h"

int main(int /*argc*/, char* /*argv*/[]) {
#ifdef FLAPPY_SDL_MAIN_HANDLED
    SDL_SetMainReady();   // 绕过 SDL2main 时必须手动告知 SDL：入口已就绪
#endif

    try {
        flappy::Game game;
        game.run();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[flappy] fatal: %s\n", e.what());
        return 1;
    }
    return 0;
}
