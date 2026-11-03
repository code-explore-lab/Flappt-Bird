// =============================================================================
//  game.cpp —— Game 的实现：资源初始化、状态机、更新、渲染、主循环
// =============================================================================
#include "game.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>
#include <utility>
#include <vector>

namespace flappy {
namespace {

// -----------------------------------------------------------------------------
// 云朵（纯装饰，缓慢飘动）
// -----------------------------------------------------------------------------
struct CloudSpec {
    float baseX = 0.0f;
    float y     = 0.0f;
    float speed = 0.0f;   // 像素 / 毫秒
    int   w     = 80;
};

constexpr std::array<CloudSpec, 3> kClouds{{
    {  60.0f,  70.0f, 0.004f,  90},
    { 300.0f, 130.0f, 0.007f,  70},
    { 520.0f,  45.0f, 0.005f, 110},
}};

// -----------------------------------------------------------------------------
// 字体查找：工程内字体优先，其次系统中文字体，最后工程自带兜底字体。
// 全部路径都是“候选”，不存在会静默跳过，因此不需要任何平台宏。
// -----------------------------------------------------------------------------
std::vector<std::string> fontCandidates() {
    std::vector<std::string> paths;

    // 0) 环境变量显式指定（方便调试 / 指定中文艺术字）
    if (const char* env = std::getenv("FLAPPY_FONT")) {
        if (*env != '\0') paths.emplace_back(env);
    }

    // 1) 可执行文件同目录（CMake 会把 assets/ 复制到这里）
    std::string baseDir;
    if (char* base = SDL_GetBasePath()) {
        baseDir = base;
        SDL_free(base);
        paths.push_back(baseDir + "assets/font.ttf");
        paths.push_back(baseDir + "font.ttf");
    }

    // 2) 当前工作目录（直接 g++ 编译后从工程根目录运行的情况）
    paths.push_back("assets/font.ttf");
    paths.push_back("font.ttf");

    // 3) 系统字体：优先带中文字形的，找不到再退到英文字体
    const char* systemFonts[] = {
        // ---- Windows ----
        "C:/Windows/Fonts/msyh.ttc",          // 微软雅黑
        "C:/Windows/Fonts/msyhbd.ttc",
        "C:/Windows/Fonts/simhei.ttf",        // 黑体
        "C:/Windows/Fonts/simsun.ttc",        // 宋体
        "C:/Windows/Fonts/arial.ttf",
        // ---- macOS ----
        "/System/Library/Fonts/PingFang.ttc",
        "/System/Library/Fonts/STHeiti Light.ttc",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        // ---- Linux ----
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/opentype/noto/NotoSansCJKsc-Regular.otf",
        "/usr/share/fonts/truetype/wqy/wqy-microhei.ttc",
        "/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
    };
    for (const char* path : systemFonts) paths.emplace_back(path);

    // 4) 工程自带兜底字体（放在最后：有系统中文字体时优先用系统的，界面才会是中文）
    paths.push_back("assets/DejaVuSans.ttf");
    if (!baseDir.empty()) paths.push_back(baseDir + "assets/DejaVuSans.ttf");
    return paths;
}

// 是否支持中文：用“游/分”两个字探测字形表（SDL2_ttf 2.0.18+ 才有该接口）
#if defined(SDL_TTF_VERSION_ATLEAST)
#  if SDL_TTF_VERSION_ATLEAST(2, 0, 18)
#    define FLAPPY_TTF_HAS_GLYPH_QUERY 1
#  endif
#endif

bool fontSupportsChinese(TTF_Font* font) {
#ifdef FLAPPY_TTF_HAS_GLYPH_QUERY
    if (!font) return false;
    return TTF_GlyphIsProvided(font, 0x6E38) != 0 &&   // 游
           TTF_GlyphIsProvided(font, 0x5206) != 0;     // 分
#else
    (void)font;
    return false;   // 老版本 SDL2_ttf 无探测接口，统一走英文界面
#endif
}

}  // namespace

// -----------------------------------------------------------------------------
// 构造：初始化 SDL / TTF、创建窗口与渲染器、加载字体
// -----------------------------------------------------------------------------
Game::Game()
    : sdl_(),
      ttf_(),
      window_(nullptr, &SDL_DestroyWindow),
      renderer_(nullptr, &SDL_DestroyRenderer),
      hudFont_(nullptr, &TTF_CloseFont),
      bigFont_(nullptr, &TTF_CloseFont) {
    // 随机种子：每次运行的管道位置都不同
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    window_.reset(SDL_CreateWindow(WINDOW_TITLE,
                                   SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                   WINDOW_W, WINDOW_H, SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI));
    if (!window_) {
        throw std::runtime_error(std::string("[flappy] SDL_CreateWindow failed: ") + SDL_GetError());
    }

    // 逻辑分辨率缩放到实际像素时使用线性过滤（Retina / 系统缩放更平滑）
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

    // 优先加速 + 垂直同步，失败则逐级降级，保证在虚拟机/远程桌面里也能跑
    renderer_.reset(SDL_CreateRenderer(window_.get(), -1,
                                       SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC));
    if (!renderer_) {
        renderer_.reset(SDL_CreateRenderer(window_.get(), -1, SDL_RENDERER_ACCELERATED));
    }
    if (!renderer_) {
        renderer_.reset(SDL_CreateRenderer(window_.get(), -1, SDL_RENDERER_SOFTWARE));
    }
    if (!renderer_) {
        throw std::runtime_error(std::string("[flappy] SDL_CreateRenderer failed: ") + SDL_GetError());
    }

    SDL_SetRenderDrawBlendMode(renderer_.get(), SDL_BLENDMODE_BLEND);
    // 逻辑分辨率固定为 640x480：Windows 缩放 / macOS Retina 下坐标不用改
    SDL_RenderSetLogicalSize(renderer_.get(), WINDOW_W, WINDOW_H);

    loadFonts();
    reset();
}

bool Game::loadFonts() {
    for (const std::string& path : fontCandidates()) {
        FontPtr hud(TTF_OpenFont(path.c_str(), HUD_POINT_SIZE), &TTF_CloseFont);
        if (!hud) continue;
        FontPtr big(TTF_OpenFont(path.c_str(), TITLE_POINT_SIZE), &TTF_CloseFont);
        if (!big) continue;

        hudFont_ = std::move(hud);
        bigFont_ = std::move(big);
        fontPath_ = path;
        chinese_ = fontSupportsChinese(hudFont_.get());
        std::printf("[flappy] font: %s (%s UI)\n", fontPath_.c_str(), chinese_ ? "Chinese" : "English");
        return true;
    }

    // 字体缺失时降级：不显示文字，但游戏照常可玩（按 README 放置字体即可恢复）
    std::fprintf(stderr,
                 "[flappy] warning: no usable TTF font found, HUD text disabled.\n"
                 "         put a .ttf into assets/font.ttf or set FLAPPY_FONT=<path>.\n");
    return false;
}

// -----------------------------------------------------------------------------
// 状态重置：小鸟、管道、分数、距离、提示计时全部回到初始值
// -----------------------------------------------------------------------------
void Game::reset() {
    bird_.reset();
    pipes_.reset();
    state_      = State::Ready;
    score_      = 0;
    meters_     = 0;
    distancePx_ = 0.0f;
    playTime_   = 0.0f;
    refreshLabels();
}

// -----------------------------------------------------------------------------
// 一帧逻辑。dt 为秒；用 dt*FPS 归一化成“60FPS 下的一帧”，
// 这样即使以后改成可变步长，手感也和固定步长一致。
// -----------------------------------------------------------------------------
void Game::update(float dt) {
    const float step = dt * static_cast<float>(FPS);   // 固定步长时恰好等于 1.0

    if (state_ == State::Ready) {
        // 待机：小鸟原地上下浮动，管道和距离都不动，等玩家按空格
        playTime_ += dt;
        bird_.y  = BIRD_START_Y + std::sin(playTime_ * 3.5f) * 6.0f;
        bird_.vy = 0.0f;
    } else if (state_ == State::Playing) {
        playTime_ += dt;

        // --- 小鸟物理 ---
        bird_.vy += GRAVITY * step;
        if (bird_.vy > MAX_FALL_SPEED) bird_.vy = MAX_FALL_SPEED;
        bird_.y += bird_.vy * step;

        // --- 管道推进 + 计分 ---
        const float dx = PIPE_SPEED * step;
        distancePx_ += dx;
        meters_ = static_cast<int>(distancePx_ / static_cast<float>(PIXELS_PER_METER));
        pipes_.update(dx);
        score_ += pipes_.collectPassed(static_cast<int>(std::lround(bird_.x)));

        // --- 碰撞：先边界，后管道 ---
        if (bird_.y < 0.0f) {                              // 撞到屏幕顶部
            bird_.y = 0.0f;
            die();
        } else if (bird_.y + static_cast<float>(bird_.h) > static_cast<float>(GROUND_Y)) {
            bird_.y = static_cast<float>(GROUND_Y - bird_.h);   // 撞到地面
            die();
        } else if (pipes_.collides(bird_.aabb())) {        // 撞到上/下管道
            die();
        }
    } else {
        // GameOver：世界静止，只让小鸟继续坠落到地面（纯表现，不影响判定）
        bird_.vy = std::min(bird_.vy + GRAVITY * step, MAX_FALL_SPEED);
        bird_.y += bird_.vy * step;
        if (bird_.y + static_cast<float>(bird_.h) > static_cast<float>(GROUND_Y)) {
            bird_.y  = static_cast<float>(GROUND_Y - bird_.h);
            bird_.vy = 0.0f;
        }
    }

    refreshLabels();
}

void Game::start() {
    state_    = State::Playing;
    playTime_ = 0.0f;
    flap();          // 起手给一个向上的速度脉冲
    refreshLabels();
}

void Game::flap() {
    bird_.vy = JUMP_VELOCITY;   // 直接给速度脉冲，连按也不会叠加
}

void Game::die() {
    if (state_ == State::GameOver) return;
    state_ = State::GameOver;
    std::printf("[flappy] game over: score=%d, distance=%d m\n", score_, meters_);
    std::fflush(stdout);
    refreshLabels();
}

void Game::onKeyDown(SDL_Keycode key) {
    switch (state_) {
        case State::Ready:
            if (key == SDLK_SPACE || key == SDLK_UP) start();   // 空格 / 上方向键开始
            break;
        case State::Playing:
            if (key == SDLK_SPACE || key == SDLK_UP) flap();    // 空格 / 上方向键上升
            break;
        case State::GameOver:
            if (key == SDLK_r) reset();                         // R 回到待机界面
            break;
    }
}

// -----------------------------------------------------------------------------
// 渲染一帧：天空 → 云 → 管道 → 地面 → 小鸟 → 结束遮罩 → HUD
// -----------------------------------------------------------------------------
void Game::render(bool present) {
    SDL_Renderer* renderer = renderer_.get();

    SDL_SetRenderDrawColor(renderer, COLOR_SKY.r, COLOR_SKY.g, COLOR_SKY.b, COLOR_SKY.a);
    SDL_RenderClear(renderer);

    renderClouds(renderer);
    pipes_.render(renderer);
    renderGround(renderer);
    bird_.render(renderer, SDL_GetTicks());

    // 结束遮罩画在世界之上、HUD 之下：HUD 保持清晰可读
    if (state_ == State::GameOver) {
        renderGameOver(renderer);
    }

    renderHud(renderer);

    if (present) SDL_RenderPresent(renderer);
}

void Game::renderClouds(SDL_Renderer* renderer) {
    const Uint32 ticks = SDL_GetTicks();
    const float range  = static_cast<float>(WINDOW_W + 240);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 150);
    for (const CloudSpec& cloud : kClouds) {
        float fx = std::fmod(cloud.baseX + static_cast<float>(ticks) * cloud.speed, range);
        if (fx < 0.0f) fx += range;
        const int x = static_cast<int>(fx) - 120;
        const int y = static_cast<int>(cloud.y);

        SDL_Rect a{x, y, cloud.w, 18};
        SDL_Rect b{x + cloud.w / 4, y - 12, cloud.w / 2, 18};
        SDL_Rect c{x + cloud.w / 8, y + 12, cloud.w * 3 / 4, 10};
        SDL_RenderFillRect(renderer, &a);
        SDL_RenderFillRect(renderer, &b);
        SDL_RenderFillRect(renderer, &c);
    }
}

void Game::renderGround(SDL_Renderer* renderer) {
    SDL_Rect soil{0, GROUND_Y, WINDOW_W, GROUND_H};
    SDL_SetRenderDrawColor(renderer, COLOR_GROUND.r, COLOR_GROUND.g, COLOR_GROUND.b, 255);
    SDL_RenderFillRect(renderer, &soil);

    SDL_Rect grass{0, GROUND_Y, WINDOW_W, 10};
    SDL_SetRenderDrawColor(renderer, COLOR_GRASS.r, COLOR_GRASS.g, COLOR_GRASS.b, 255);
    SDL_RenderFillRect(renderer, &grass);

    // 随距离滚动的草纹：强化“一直在向前跑”的感觉
    constexpr int kSpacing = 32;
    const int offset = static_cast<int>(distancePx_) % kSpacing;
    SDL_SetRenderDrawColor(renderer, 90, 165, 75, 255);
    for (int x = -offset; x < WINDOW_W; x += kSpacing) {
        SDL_Rect dash{x + 8, GROUND_Y + 14, 6, 4};
        SDL_RenderFillRect(renderer, &dash);
    }

    SDL_SetRenderDrawColor(renderer, 120, 105, 70, 255);
    SDL_Rect line{0, GROUND_Y, WINDOW_W, 2};
    SDL_RenderFillRect(renderer, &line);
}

void Game::renderHud(SDL_Renderer* renderer) {
    if (!hudFont_) return;

    constexpr int kMargin = 16;
    scoreLabel_.drawShadowed(renderer, WINDOW_W - kMargin, 12, Align::Right);
    distanceLabel_.drawShadowed(renderer, WINDOW_W - kMargin, 46, Align::Right);

    // 待机提示：按空格开始（游戏开始后自动消失）
    if (state_ == State::Ready) {
        hintLabel_.drawShadowed(renderer, WINDOW_W / 2, 300, Align::Center);
    }
}

void Game::renderGameOver(SDL_Renderer* renderer) {
    // 半透明遮罩
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 115);
    SDL_Rect full{0, 0, WINDOW_W, WINDOW_H};
    SDL_RenderFillRect(renderer, &full);

    // 居中面板
    SDL_Rect panel{WINDOW_W / 2 - 210, 120, 420, 210};
    SDL_SetRenderDrawColor(renderer, 22, 32, 42, 195);
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 70);
    SDL_RenderDrawRect(renderer, &panel);

    overTitleLabel_.drawShadowed(renderer, WINDOW_W / 2, 142, Align::Center);
    overScoreLabel_.drawShadowed(renderer, WINDOW_W / 2, 214, Align::Center);

    // “按 R 重开”闪烁提示：最暗也保持可读
    const bool visible = ((SDL_GetTicks() / 450) % 2) == 0;
    overPromptLabel_.drawShadowed(renderer, WINDOW_W / 2, 278, Align::Center, visible ? 255 : 130);
}

// -----------------------------------------------------------------------------
// 文本刷新：字符串内容不变时 Label 会复用旧纹理，所以可以安心每帧调用
// -----------------------------------------------------------------------------
void Game::refreshLabels() {
    if (!hudFont_) return;

    SDL_Renderer* renderer = renderer_.get();
    const std::string scoreText = tr("分数 ", "SCORE ") + std::to_string(score_);
    const std::string distText  = tr("距离 ", "DIST ") + std::to_string(meters_) + tr(" 米", " m");

    scoreLabel_.set(renderer, hudFont_.get(), scoreText, COLOR_TEXT);
    distanceLabel_.set(renderer, hudFont_.get(), distText, COLOR_TEXT);
    hintLabel_.set(renderer, bigFont_.get(), tr("按空格开始", "PRESS SPACE TO START"), COLOR_TEXT);

    if (state_ == State::GameOver) {
        overTitleLabel_.set(renderer, bigFont_.get(), tr("游戏结束", "GAME OVER"), COLOR_TEXT);
        overScoreLabel_.set(renderer, hudFont_.get(), scoreText + "    " + distText, COLOR_TEXT);
        overPromptLabel_.set(renderer, hudFont_.get(), tr("按 R 键重新开始", "PRESS R TO RESTART"), COLOR_TEXT);
    }
}

// -----------------------------------------------------------------------------
// 主循环：固定 60FPS 上限 + 固定物理步长累加器
//   * 用 SDL_GetTicks 计算帧间隔，累加到 accumulator
//   * 每积满 1/60 秒就推进一次物理，保证不同刷新率下手感一致
//   * 单帧最多补 5 步，防止卡顿后“瞬移”
// -----------------------------------------------------------------------------
void Game::run() {
    Uint32 lastTicks   = SDL_GetTicks();
    float  accumulator = 0.0f;
    bool   running     = true;

    while (running) {
        const Uint32 frameStart = SDL_GetTicks();
        Uint32 deltaMs = frameStart - lastTicks;
        lastTicks = frameStart;
        if (deltaMs > 250) deltaMs = 250;   // 窗口被拖动/系统休眠后的巨大跳变

        // ---- 事件 ----
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.repeat != 0) continue;            // 忽略系统按键重复
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                } else {
                    onKeyDown(event.key.keysym.sym);
                }
            }
        }
        if (!running) break;

        // ---- 固定步长更新 ----
        accumulator += static_cast<float>(deltaMs) / 1000.0f;
        int steps = 0;
        while (accumulator >= FIXED_DT && steps < 5) {
            update(FIXED_DT);
            accumulator -= FIXED_DT;
            ++steps;
        }
        if (steps == 5) accumulator = 0.0f;

        // ---- 渲染 ----
        render();

        // ---- 帧率上限 ----
        const Uint32 elapsed = SDL_GetTicks() - frameStart;
        if (elapsed < static_cast<Uint32>(FRAME_MS)) {
            SDL_Delay(static_cast<Uint32>(FRAME_MS) - elapsed);
        }
    }
}

}  // namespace flappy
