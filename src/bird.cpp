// =============================================================================
//  bird.cpp —— Bird 的实现
// =============================================================================
#include "bird.h"

#include <cmath>

namespace flappy {

void Bird::reset() {
    x  = BIRD_START_X;
    y  = BIRD_START_Y;
    vy = 0.0f;
}

SDL_Rect Bird::aabb() const {
    return SDL_Rect{
        static_cast<int>(std::lround(x)) + BIRD_HIT_INSET,
        static_cast<int>(std::lround(y)) + BIRD_HIT_INSET,
        w - BIRD_HIT_INSET * 2,
        h - BIRD_HIT_INSET * 2};
}

void Bird::render(SDL_Renderer* renderer, Uint32 ticks) const {
    const int bx = static_cast<int>(std::lround(x));
    const int by = static_cast<int>(std::lround(y));

    // 翅膀每 120ms 换一个姿态，做出扇动感
    const int wingPhase  = static_cast<int>((ticks / 120) % 3);
    const int wingOffset = (wingPhase == 0) ? 0 : (wingPhase == 1 ? -3 : 3);

    // 身体（外圈描边 + 内层黄色）
    SDL_SetRenderDrawColor(renderer, 180, 120, 0, 255);
    SDL_Rect outline{bx, by, w, h};
    SDL_RenderFillRect(renderer, &outline);

    SDL_SetRenderDrawColor(renderer, 255, 220, 0, 255);
    SDL_Rect body{bx + 2, by + 2, w - 4, h - 4};
    SDL_RenderFillRect(renderer, &body);

    // 翅膀
    SDL_SetRenderDrawColor(renderer, 255, 245, 160, 255);
    SDL_Rect wing{bx + 7, by + h / 2 - 4 + wingOffset, 12, 7};
    SDL_RenderFillRect(renderer, &wing);

    // 眼睛
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_Rect eye{bx + w - 15, by + 4, 9, 9};
    SDL_RenderFillRect(renderer, &eye);

    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_Rect pupil{bx + w - 11, by + 6, 4, 4};
    SDL_RenderFillRect(renderer, &pupil);

    // 喙（伸出身体右侧一点点）
    SDL_SetRenderDrawColor(renderer, 255, 150, 0, 255);
    SDL_Rect beak{bx + w - 2, by + h / 2 - 3, 9, 7};
    SDL_RenderFillRect(renderer, &beak);
    SDL_Rect beak2{bx + w - 2, by + h / 2 + 2, 6, 4};
    SDL_RenderFillRect(renderer, &beak2);
}

}  // namespace flappy
