// =============================================================================
//  pipe.h —— 管道与管道对象池
// =============================================================================
#pragma once

#include <SDL.h>

#include <array>

#include "config.h"

namespace flappy {

struct Pipe {
    float x      = 0.0f;    // 左边缘
    int   gapY   = 0;       // 缺口顶部 y
    bool  scored = false;   // 这一对是否已经计过分

    SDL_Rect topRect() const;
    SDL_Rect bottomRect() const;
    void render(SDL_Renderer* renderer) const;
};

class PipeManager {
public:
    // 回到初始布局：三对管道等间距排在屏幕右侧外
    void reset();

    // 整体左移；完全移出屏幕后重置到最右侧并重新随机缺口（对象池循环复用）
    void update(float dx);

    // 返回本帧新穿越的管道对数（每对只计一次分）
    int collectPassed(int birdX);

    // AABB 相交：使用 SDL_HasIntersection 检测小鸟与上/下管段
    bool collides(const SDL_Rect& box) const;

    void render(SDL_Renderer* renderer) const;

private:
    // 缺口随机位置：在上一对管道的缺口基础上平滑变化，避免出现“必须瞬移”的落差
    void randomizeGap(Pipe& pipe);

    std::array<Pipe, PIPE_COUNT> pipes_{};   // 固定池：运行期不分配、不扩容
    int lastGapY_ = 0;                       // 最右侧（最新生成）管道的缺口顶部
};

}  // namespace flappy
