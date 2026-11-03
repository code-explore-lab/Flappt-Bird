// =============================================================================
//  pipe.cpp —— Pipe / PipeManager 的实现
// =============================================================================
#include "pipe.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

#include "util.h"

namespace flappy {

SDL_Rect Pipe::topRect() const {
    return SDL_Rect{static_cast<int>(std::lround(x)), 0, PIPE_W, gapY};
}

SDL_Rect Pipe::bottomRect() const {
    const int top = gapY + PIPE_GAP_H;
    return SDL_Rect{static_cast<int>(std::lround(x)), top, PIPE_W, GROUND_Y - top};
}

void Pipe::render(SDL_Renderer* renderer) const {
    const SDL_Rect top    = topRect();
    const SDL_Rect bottom = bottomRect();
    if (top.x > WINDOW_W || top.x + PIPE_W < 0) return;   // 屏幕外不画

    constexpr int kCapH = 20;        // 管口（帽檐）高度
    constexpr int kCapOverhang = 4;  // 管口左右各外扩像素

    auto fill = [renderer](const SDL_Rect& rect, SDL_Color color) {
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_RenderFillRect(renderer, &rect);
    };

    // 管身
    fill(top, COLOR_PIPE);
    fill(bottom, COLOR_PIPE);

    // 左侧高光 + 右侧阴影，制造一点立体感
    fill(SDL_Rect{top.x + 8, top.y, 6, top.h}, COLOR_PIPE_LIGHT);
    fill(SDL_Rect{top.x + PIPE_W - 10, top.y, 10, top.h}, COLOR_PIPE_DARK);
    fill(SDL_Rect{bottom.x + 8, bottom.y, 6, bottom.h}, COLOR_PIPE_LIGHT);
    fill(SDL_Rect{bottom.x + PIPE_W - 10, bottom.y, 10, bottom.h}, COLOR_PIPE_DARK);

    // 管口
    const SDL_Rect capTop   {top.x - kCapOverhang,    gapY - kCapH, PIPE_W + kCapOverhang * 2, kCapH};
    const SDL_Rect capBottom{bottom.x - kCapOverhang, bottom.y,     PIPE_W + kCapOverhang * 2, kCapH};
    fill(capTop, COLOR_PIPE);
    fill(capBottom, COLOR_PIPE);
    fill(SDL_Rect{capTop.x + 4, capTop.y, 6, capTop.h}, COLOR_PIPE_LIGHT);
    fill(SDL_Rect{capBottom.x + 4, capBottom.y, 6, capBottom.h}, COLOR_PIPE_LIGHT);
    fill(SDL_Rect{capTop.x + capTop.w - 10, capTop.y, 10, capTop.h}, COLOR_PIPE_DARK);
    fill(SDL_Rect{capBottom.x + capBottom.w - 10, capBottom.y, 10, capBottom.h}, COLOR_PIPE_DARK);
}

void PipeManager::reset() {
    constexpr int maxGapY = GROUND_Y - PIPE_MIN_BOTTOM - PIPE_GAP_H;

    // 第一对管道尽量对准小鸟的起始高度，开局不用立刻大幅爬升/俯冲
    const int startCenter = static_cast<int>(BIRD_START_Y) + BIRD_H / 2;
    lastGapY_ = std::clamp(startCenter - PIPE_GAP_H / 2 + randomInt(-30, 30),
                           PIPE_MIN_TOP, maxGapY);

    for (int i = 0; i < PIPE_COUNT; ++i) {
        Pipe& p = pipes_[static_cast<std::size_t>(i)];
        p.x      = static_cast<float>(WINDOW_W + 160 + i * PIPE_SPACING);
        p.scored = false;
        if (i == 0) {
            p.gapY = lastGapY_;
        } else {
            randomizeGap(p);   // 后面几对在上一次的基础上平滑随机
        }
    }
}

void PipeManager::update(float dx) {
    float rightmost = -1.0e9f;
    for (const Pipe& p : pipes_) rightmost = std::max(rightmost, p.x);

    for (Pipe& p : pipes_) {
        p.x -= dx;
        if (p.x + static_cast<float>(PIPE_W) < 0.0f) {
            p.x      = rightmost + static_cast<float>(PIPE_SPACING);
            p.scored = false;
            randomizeGap(p);
            rightmost = p.x;   // 同一帧回收多根时，保证它们依次向右排开
        }
    }
}

int PipeManager::collectPassed(int birdX) {
    int passed = 0;
    const float birdLeft = static_cast<float>(birdX);
    for (Pipe& p : pipes_) {
        if (!p.scored && p.x + static_cast<float>(PIPE_W) < birdLeft) {
            p.scored = true;
            ++passed;
        }
    }
    return passed;
}

bool PipeManager::collides(const SDL_Rect& box) const {
    for (const Pipe& p : pipes_) {
        const SDL_Rect top    = p.topRect();
        const SDL_Rect bottom = p.bottomRect();
        if (SDL_HasIntersection(&box, &top) || SDL_HasIntersection(&box, &bottom)) {
            return true;
        }
    }
    return false;
}

void PipeManager::render(SDL_Renderer* renderer) const {
    for (const Pipe& p : pipes_) p.render(renderer);
}

void PipeManager::randomizeGap(Pipe& pipe) {
    constexpr int maxGapY = GROUND_Y - PIPE_MIN_BOTTOM - PIPE_GAP_H;  // 440-80-155 = 205
    const int lo = std::max(PIPE_MIN_TOP, lastGapY_ - PIPE_GAP_MAX_STEP);
    const int hi = std::min(maxGapY, lastGapY_ + PIPE_GAP_MAX_STEP);
    pipe.gapY = randomInt(lo, hi);
    lastGapY_ = pipe.gapY;
}

}  // namespace flappy
