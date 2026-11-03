# DESIGN.md — Flappy Bird 风格跑酷小游戏

> 版本：1.0
> 技术栈：C++17 + SDL2（+ SDL2_ttf，可选 SDL2_image / SDL2_mixer）
> 目标平台：Windows / Linux / macOS
> 目标：跨平台、可编译、可玩的最小完整 2D 跑酷游戏，并保留升级贴图/音效的接口。

---

## 1. 项目概述

### 1.1 一句话描述
玩家控制一只受重力下落的小鸟，按 Space 向上飞，穿过从右向左移动的管道，穿过得分，撞管道或边界即结束，右上角显示分数/距离，可重开。

### 1.2 目标用户
- 想学习 SDL2 + C++ 的开发者；
- 需要一个可玩的最小 2D 游戏骨架作为二次开发基础。

### 1.3 非目标（本项目不做）
- 不做 3D、不做物理引擎、不做联网、不做关卡编辑器；
- 不做复杂 UI 框架、不做资源热重载；
- 不追求像素级完美碰撞（用 AABB 矩形即可）。

---

## 2. 玩法设计

### 2.1 核心循环
1. 小鸟受重力持续下落；
2. 玩家按 Space 给小鸟一个向上的速度脉冲；
3. 管道对从右向左匀速移动，每对有上下两段，中间留缺口；
4. 小鸟穿过一对管道 → 分数 +1；
5. 小鸟撞到管道或上下边界 → 游戏结束；
6. 结束后按 R 重置并重新开始。

### 2.2 规则细节
| 规则 | 说明 |
|---|---|
| 重力 | 每帧 `vy += gravity`，`y += vy` |
| 跳跃 | Space：`vy = jumpVelocity`（负值，向上） |
| 管道移动 | 每帧 `x -= speed` |
| 得分判定 | 管道右边缘越过小鸟左边缘且未计分 → `score++`，标记 `scored` |
| 碰撞判定 | 小鸟 AABB 与上/下管道 AABB 相交，或小鸟超出上下边界 |
| 重开 | gameover 状态下按 R：重置小鸟、管道、分数、状态 |
| 距离（可选） | 累计 `distance += speed`，每 100 像素记 1 米，右上角显示 |

### 2.3 可调参数（建议集中为常量）
```cpp
constexpr int   WINDOW_W      = 640;
constexpr int   WINDOW_H      = 480;
constexpr int   FPS           = 60;
constexpr float GRAVITY       = 0.5f;    // px/frame^2
constexpr float JUMP_VELOCITY = -8.0f;   // px/frame
constexpr float PIPE_SPEED    = 3.0f;    // px/frame
constexpr int   PIPE_W        = 60;
constexpr int   PIPE_GAP_H    = 120;     // 缺口高度
constexpr int   PIPE_COUNT    = 3;       // 复用管道数量
constexpr int   PIPE_SPACING  = 220;     // 管道水平间距
constexpr int   BIRD_W        = 34;
constexpr int   BIRD_H        = 24;
constexpr float BIRD_START_X  = 100.0f;
constexpr float BIRD_START_Y  = 240.0f;
手感调优建议：先固定 60 FPS + 每帧固定增量；后续可改成 deltaTime 缩放，保证不同刷新率一致。

3. 系统架构
3.1 分层
text
+---------------------------+
|        main / Game        |  主循环、事件分发、状态切换
+---------------------------+
|  Bird | PipeManager | HUD  |  实体与表现
+---------------------------+
|  Renderer | Input | Audio  |  平台能力封装（SDL 封装）
+---------------------------+
|           SDL2            |  窗口/渲染/事件/字体/音频
+---------------------------+
3.2 模块划分
模块	职责	关键类型/文件
Game	主循环、状态机、更新/渲染调度	Game（game.cpp/h）
Bird	小鸟位置、速度、重力、跳跃、碰撞盒	Bird
PipeManager	管道池、移动、复用、生成、计分、碰撞盒	PipeManager, Pipe
Renderer	封装 SDL_Renderer，画矩形/贴图/文字	Renderer
Input	封装 SDL_Event → 语义事件（Jump/Restart/Quit）	Input
HUD	右上角分数/距离、Game Over 提示	HUD
Resources（可选）	字体/贴图/音效加载与 RAII 释放	Resources
Config	常量参数集中	config.h
最小版可把 Renderer/Input/HUD 内联在 Game 里，但建议保留清晰边界，便于升级。

3.3 主循环（固定 60 FPS）
text
while (running):
    frameStart = SDL_GetTicks()
    input.poll()            // 处理事件，产出 Jump/Restart/Quit
    game.update(dt)         // 按状态更新
    game.render(renderer)   // 清屏 -> 画管道 -> 画小鸟 -> 画 HUD -> Present
    frameTime = SDL_GetTicks() - frameStart
    if frameTime < FRAME_DELAY: SDL_Delay(FRAME_DELAY - frameTime)
最小版用"每帧固定增量 + 固定 FPS"；

升级版用 dt = frameTime / 1000.0f，所有速度乘 dt * 60 归一化，保证高刷屏手感一致。

4. 游戏状态机
text
        +--------+
        | Playing|<-----------+
        +---+----+            |
            | 碰撞/越界         | R 重置
            v                  |
        +--------+             |
        |GameOver+-------------+
        +--------+
状态	行为
Playing	更新物理、管道、计分、碰撞；Space 可跳；渲染游戏世界 + HUD
GameOver	停止更新世界；渲染最后一帧 + "Game Over / 按 R 重开"；R 触发重置回 Playing
重置内容：小鸟回初始位置与速度、管道全部重置到初始分布、score/distance 归零、状态回 Playing。

可选增加 Menu 态（启动画面），非必需。

5. 数据结构
cpp
struct Bird {
    float x = BIRD_START_X;
    float y = BIRD_START_Y;
    float vy = 0.0f;
    int   w = BIRD_W;
    int   h = BIRD_H;
    SDL_Rect aabb() const { return { (int)x, (int)y, w, h }; }
};

struct Pipe {
    float x;          // 左边缘
    int   gapY;       // 缺口顶部 y
    int   gapH;       // 缺口高度
    int   w;          // 宽度
    bool  scored;     // 是否已计分
    SDL_Rect topRect() const { return { (int)x, 0, w, gapY }; }
    SDL_Rect bottomRect() const {
        return { (int)x, gapY + gapH, w, WINDOW_H - (gapY + gapH) };
    }
};

class PipeManager {
    std::vector<Pipe> pipes_;   // 固定 PIPE_COUNT 个，循环复用
public:
    void reset();               // 初始化等间距分布
    void update(float speed);   // 左移 + 复用 + 计分回调
    bool collide(const Bird&) const;
    void render(Renderer&) const;
    int  score() const;
};
管道复用：不 push_back 新对象，移出屏幕后 x = 最右管道 x + PIPE_SPACING，gapY 重新随机，scored = false。

6. 碰撞检测
采用 AABB（轴对齐矩形）相交，使用 SDL_HasIntersection。

检测对象：

小鸟 AABB vs 每根管道上段 AABB；
小鸟 AABB vs 每根管道下段 AABB；
小鸟 AABB vs 窗口上下边界（y < 0 或 y + h > WINDOW_H）。
任一项命中 → 进入 GameOver。

说明：矩形碰撞对小游戏足够；如后续要更精确可换圆形/像素级，但不在本设计范围。

7. 渲染设计
7.1 绘制顺序（每帧）
清屏为天空蓝（135,206,235）；

画管道（绿色 0,180,0）——先画远的（右侧）或统一顺序即可，2D 无遮挡问题；

画小鸟（黄色 255,220,0）；

画 HUD 文字（白色）；

若 GameOver，叠加半透明遮罩 + 提示文字；

SDL_RenderPresent。

7.2 最小版（纯色方块）
全部用 SDL_RenderFillRect；

文字用 SDL2_ttf：TTF_RenderUTF8_Blended → SDL_CreateTextureFromSurface → SDL_RenderCopy，用后 SDL_FreeSurface + SDL_DestroyTexture。

7.3 贴图版（可选升级）
用 SDL2_image 加载 png：小鸟、管道、背景、地面；

小鸟用 SDL_RenderCopyEx 按 vy 旋转角度（向上/向下倾斜），增强手感；

地面滚动：底部长条贴图每帧左移并循环，强化"跑酷"感；

资源目录：assets/bird.png、assets/pipe.png、assets/background.png、assets/font.ttf。

7.4 HUD
右上角：Score: N（必做）；Distance: M m（可选）；

GameOver 居中：Game Over + Press R to Restart；

字体：TTF_OpenFont("assets/font.ttf", 24)，需支持中文时用含中文字形的字体并以 UTF-8 渲染。

8. 资源与依赖
资源/依赖	用途	是否必需
SDL2	窗口/渲染/事件/计时	必需
SDL2_ttf	文字（分数/提示）	必需（最小版即用）
SDL2_image	加载 png 贴图	可选（贴图版）
SDL2_mixer	音效/音乐	可选
font.ttf	字体文件	必需（放 exe 同目录或 assets/）
所有 SDL 资源用 RAII 封装（自定义 deleter 的 unique_ptr）或显式 Destroy，SDL_Quit 在退出时调用。

字体缺失要有兜底：若 TTF_OpenFont 失败，退化为不显示文字但游戏可玩，并在控制台警告。

9. 构建与跨平台
9.1 目录结构（建议）
text
flappy/
  CMakeLists.txt
  DESIGN.md
  README.md
  src/
    main.cpp
    game.cpp / game.h
    bird.h
    pipe.h / pipe.cpp
    renderer.h / renderer.cpp
    input.h / input.cpp
    hud.h / hud.cpp
    config.h
  assets/
    font.ttf
    (可选) bird.png pipe.png background.png
9.2 CMakeLists.txt（跨平台）
cmake_minimum_required(VERSION 3.16)，CXX_STANDARD 17；

find_package(SDL2 REQUIRED)、find_package(SDL2_ttf REQUIRED)；

target_link_libraries(flappy PRIVATE SDL2::SDL2 SDL2::SDL2main SDL2_ttf::SDL2_ttf)；

可选：SDL2_image::SDL2_image、SDL2_mixer::SDL2_mixer；

构建后把 dll/资源复制到输出目录（Windows 用 add_custom_command 复制 SDL2.dll、assets/）。

9.3 各平台命令
Ubuntu

bash
sudo apt install build-essential libsdl2-dev libsdl2-ttf-dev
# 可选：libsdl2-image-dev libsdl2-mixer-dev
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build
# 或直接：
g++ src/*.cpp -o flappy $(pkg-config --cflags --libs sdl2 SDL2_ttf)
Windows（MinGW-w64）

bash
g++ src/*.cpp -o flappy.exe -Isdl2/include -Lsdl2/lib ^
  -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf
# 把 SDL2.dll、SDL2_ttf.dll 复制到 flappy.exe 同目录
链接顺序 -lmingw32 -lSDL2main -lSDL2 不可乱；

想隐藏控制台加 -mwindows。

macOS

bash
brew install sdl2 sdl2_ttf
cmake -B build && cmake --build build
9.4 Windows 特有坑
问题	解决
找不到 SDL2.dll	dll 放 exe 同目录 / PATH
undefined reference to WinMain	MinGW 链接 -lmingw32 -lSDL2main -lSDL2；VS 链接 SDL2main.lib + 子系统 Windows，或 #define SDL_MAIN_HANDLED + SDL_SetMainReady()
控制台窗口	MinGW -mwindows；VS 子系统 Windows
位数不匹配	x64 编译器配 x64 lib/dll
中文乱码	源码 UTF-8；ttf 用 TTF_RenderUTF8_Blended；控制台 SetConsoleOutputCP(CP_UTF8)
缺 MSVC 运行库	/MT 静态链接或装 VC++ Redistributable
10. 性能与健壮性
管道固定池，无每帧堆分配；渲染无临时纹理泄漏（文字纹理用后即毁）。

固定 60 FPS + 帧时间补偿；后续可切 deltaTime。

随机数 srand(time(nullptr)) 初始化一次。

所有 SDL 返回值检查，失败打印 SDL_GetError() 并优雅退出。

字体/贴图加载失败可降级（无文字/纯色）而非崩溃。


