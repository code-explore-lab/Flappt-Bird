// =============================================================================
//  raii.h —— SDL / SDL_ttf 上下文与资源所有权
//
//  SdlContext 在构造时 SDL_Init、析构时 SDL_Quit；
//  TtfContext 同理。窗口/渲染器/字体/纹理/Surface 全部用带 deleter 的
//  std::unique_ptr 管理，避免手写 Destroy 时漏掉分支。
// =============================================================================
#pragma once

#include <SDL.h>
#include <SDL_ttf.h>

#include <memory>
#include <stdexcept>
#include <string>

namespace flappy {

class SdlContext {
public:
    SdlContext() {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
            throw std::runtime_error(std::string("[flappy] SDL_Init failed: ") + SDL_GetError());
        }
    }
    ~SdlContext() { SDL_Quit(); }

    SdlContext(const SdlContext&) = delete;
    SdlContext& operator=(const SdlContext&) = delete;
};

class TtfContext {
public:
    TtfContext() {
        if (TTF_Init() != 0) {   // 0 = 成功
            throw std::runtime_error(std::string("[flappy] TTF_Init failed: ") + TTF_GetError());
        }
    }
    ~TtfContext() { TTF_Quit(); }

    TtfContext(const TtfContext&) = delete;
    TtfContext& operator=(const TtfContext&) = delete;
};

using WindowPtr   = std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>;
using RendererPtr = std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)>;
using FontPtr     = std::unique_ptr<TTF_Font, decltype(&TTF_CloseFont)>;
using SurfacePtr  = std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)>;
using TexturePtr  = std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>;

}  // namespace flappy
