// =============================================================================
//  config.h —— 可调参数、颜色、尺寸（改手感只需要动这里）
// =============================================================================
#pragma once

#include <SDL.h>

namespace flappy {

// ---- 窗口 / 主循环 ----
constexpr int   WINDOW_W        = 640;
constexpr int   WINDOW_H        = 480;
constexpr int   FPS             = 60;
constexpr int   FRAME_MS        = 1000 / FPS;          // 每帧预算 16ms
constexpr float FIXED_DT        = 1.0f / static_cast<float>(FPS);  // 固定物理步长（秒）

// ---- 物理（单位：像素 / 帧，按 60FPS 归一化）----
constexpr float GRAVITY         = 0.45f;   // 每帧速度增量（越小下落越慢，越好救）
constexpr float JUMP_VELOCITY   = -7.8f;   // 跳跃瞬间的速度（负数向上）
constexpr float MAX_FALL_SPEED  = 10.5f;   // 最大下落速度
constexpr float PIPE_SPEED      = 2.5f;    // 管道每帧左移像素（越小越慢）

// ---- 管道 ----
constexpr int   PIPE_W          = 60;      // 管道宽度
constexpr int   PIPE_GAP_H      = 155;     // 上下管道之间缺口高度（越大越简单）
constexpr int   PIPE_COUNT      = 3;       // 对象池大小（固定 3 对，循环复用）
constexpr int   PIPE_SPACING    = 260;     // 相邻两对管道的水平间距（越大越从容）
constexpr int   PIPE_MIN_TOP    = 60;      // 缺口上边界的最小值
constexpr int   PIPE_MIN_BOTTOM = 80;      // 缺口下边界到地面的最小距离
constexpr int   PIPE_GAP_MAX_STEP = 80;    // 相邻两对管道缺口中心的最大落差（像素）

// ---- 地面 / 边界 ----
constexpr int   GROUND_H        = 40;
constexpr int   GROUND_Y        = WINDOW_H - GROUND_H;  // 地面顶部 y = 440

// ---- 小鸟 ----
constexpr int   BIRD_W          = 34;
constexpr int   BIRD_H          = 24;
constexpr float BIRD_START_X    = 100.0f;
constexpr float BIRD_START_Y    = 200.0f;
constexpr int   BIRD_HIT_INSET  = 4;       // 判定盒四周内缩，手感更宽容

// ---- HUD / 文本 ----
constexpr int   HUD_POINT_SIZE   = 24;
constexpr int   TITLE_POINT_SIZE = 46;
constexpr int   PIXELS_PER_METER = 100;    // 100 像素 = 1 米

// ---- 颜色（纯色方块版的全部“美术资源”）----
constexpr SDL_Color COLOR_SKY        {135, 206, 235, 255};
constexpr SDL_Color COLOR_TEXT       {255, 255, 255, 255};
constexpr SDL_Color COLOR_PIPE       { 26, 168,  78, 255};
constexpr SDL_Color COLOR_PIPE_LIGHT {142, 226, 142, 255};
constexpr SDL_Color COLOR_PIPE_DARK  {  0, 110,  45, 255};
constexpr SDL_Color COLOR_GROUND     {222, 216, 149, 255};
constexpr SDL_Color COLOR_GRASS      {110, 185,  90, 255};

constexpr const char* WINDOW_TITLE = "Flappy Bird - C++17 + SDL2";

}  // namespace flappy
