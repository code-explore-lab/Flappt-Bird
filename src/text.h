// =============================================================================
//  text.h —— 带缓存的文字纹理
//
//  HUD 每帧都要画文字，但 TTF 光栅化很贵。Label 缓存“文本 + 颜色”对应的
//  纹理，内容不变时直接复用，避免每帧重建纹理。
// =============================================================================
#pragma once

#include <SDL.h>
#include <SDL_ttf.h>

#include <string>

#include "raii.h"
#include "state.h"

namespace flappy {

class Label {
public:
    Label() = default;
    Label(const Label&) = delete;
    Label& operator=(const Label&) = delete;

    // 设置文本；text/color 与上次相同则直接复用纹理
    void set(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, SDL_Color color);

    int width() const { return w_; }
    int height() const { return h_; }

    void draw(SDL_Renderer* renderer, int x, int y, Align align, Uint8 alpha = 255) const;
    // 带 2px 深色阴影，保证在天空 / 管道上都看得清
    void drawShadowed(SDL_Renderer* renderer, int x, int y, Align align, Uint8 alpha = 255) const;

private:
    TexturePtr  texture_{nullptr, &SDL_DestroyTexture};
    std::string text_;
    SDL_Color   color_{0, 0, 0, 0};
    int         w_ = 0;
    int         h_ = 0;
};

}  // namespace flappy
