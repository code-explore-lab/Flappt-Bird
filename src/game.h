// =============================================================================
//  game.h —— 游戏主体：窗口、渲染器、字体、实体、状态机与主循环
// =============================================================================
#pragma once

#include <SDL.h>
#include <SDL_ttf.h>

#include <string>

#include "bird.h"
#include "pipe.h"
#include "raii.h"
#include "state.h"
#include "text.h"

namespace flappy {

class Game {
public:
    Game();
    ~Game() = default;   // 成员都是 RAII，按声明逆序自动释放

    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    // 主循环：固定 60 FPS + 固定物理步长
    void run();

    // 单步接口（也方便写自动化测试 / 自定义主循环）
    void reset();
    void update(float dt);
    void render(bool present = true);
    void onKeyDown(SDL_Keycode key);

    // 只读访问器
    State state() const { return state_; }
    int score() const { return score_; }
    int meters() const { return meters_; }
    SDL_Renderer* renderer() const { return renderer_.get(); }
    SDL_Window* window() const { return window_.get(); }

private:
    void start();
    void flap();
    void die();
    void refreshLabels();

    void renderClouds(SDL_Renderer* renderer);
    void renderGround(SDL_Renderer* renderer);
    void renderHud(SDL_Renderer* renderer);
    void renderGameOver(SDL_Renderer* renderer);

    bool loadFonts();
    const char* tr(const char* zh, const char* en) const { return chinese_ ? zh : en; }

    // ---- RAII 资源：声明顺序即依赖顺序，析构自动逆序 ----
    SdlContext  sdl_;
    TtfContext  ttf_;
    WindowPtr   window_;
    RendererPtr renderer_;
    FontPtr     hudFont_;
    FontPtr     bigFont_;

    // ---- 游戏状态 ----
    Bird        bird_;
    PipeManager pipes_;
    State       state_       = State::Ready;
    int         score_       = 0;
    int         meters_      = 0;
    float       distancePx_  = 0.0f;   // 累计移动像素，用来换算距离
    float       playTime_    = 0.0f;   // 待机动画时间 / 本局时间
    bool        chinese_     = false;  // 当前字体能否显示中文
    std::string fontPath_;

    // ---- 文字（纹理缓存）----
    Label scoreLabel_;
    Label distanceLabel_;
    Label hintLabel_;
    Label overTitleLabel_;
    Label overScoreLabel_;
    Label overPromptLabel_;
};

}  // namespace flappy
