// =============================================================================
//  text.cpp —— Label 实现
// =============================================================================
#include "text.h"

namespace flappy {

void Label::set(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, SDL_Color color) {
    if (text_ == text && color.r == color_.r && color.g == color_.g &&
        color.b == color_.b && color.a == color_.a) {
        return;
    }
    text_ = text;
    color_ = color;
    texture_.reset();
    w_ = h_ = 0;

    if (!font || !renderer || text.empty()) return;

    SDL_Surface* raw = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!raw) return;   // 渲染失败（例如字体缺字形）时保持空纹理，不影响游戏
    SurfacePtr surface(raw, &SDL_FreeSurface);

    texture_.reset(SDL_CreateTextureFromSurface(renderer, surface.get()));
    if (!texture_) return;
    w_ = surface->w;
    h_ = surface->h;
    SDL_SetTextureBlendMode(texture_.get(), SDL_BLENDMODE_BLEND);
}

void Label::draw(SDL_Renderer* renderer, int x, int y, Align align, Uint8 alpha) const {
    if (!texture_) return;
    SDL_Rect dst{x, y, w_, h_};
    if (align == Align::Center)      dst.x -= w_ / 2;
    else if (align == Align::Right)  dst.x -= w_;

    if (alpha != 255) SDL_SetTextureAlphaMod(texture_.get(), alpha);
    SDL_RenderCopy(renderer, texture_.get(), nullptr, &dst);
    if (alpha != 255) SDL_SetTextureAlphaMod(texture_.get(), 255);
}

void Label::drawShadowed(SDL_Renderer* renderer, int x, int y, Align align, Uint8 alpha) const {
    if (!texture_) return;
    SDL_SetTextureColorMod(texture_.get(), 30, 30, 30);
    draw(renderer, x + 2, y + 2, align, alpha);
    SDL_SetTextureColorMod(texture_.get(), 255, 255, 255);
    draw(renderer, x, y, align, alpha);
}

}  // namespace flappy
