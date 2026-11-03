// =============================================================================
//  bird.h —— 小鸟：位置、速度、碰撞盒、绘制
// =============================================================================
#pragma once

#include <SDL.h>

#include "config.h"

namespace flappy {

struct Bird {
    float x  = BIRD_START_X;
    float y  = BIRD_START_Y;
    float vy = 0.0f;
    int   w  = BIRD_W;
    int   h  = BIRD_H;

    void reset();

    // 碰撞盒：比贴图略小一圈，避免“擦边就死”的挫败感
    SDL_Rect aabb() const;

    // 纯色方块画一只小鸟：身体 + 翅膀 + 眼睛 + 喙
    void render(SDL_Renderer* renderer, Uint32 ticks) const;
};

}  // namespace flappy
