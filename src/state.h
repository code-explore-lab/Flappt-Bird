// =============================================================================
//  state.h —— 游戏状态机与文本对齐
// =============================================================================
#pragma once

namespace flappy {

enum class State {
    Ready,     // 待机：小鸟上下浮动，管道静止，按空格开始
    Playing,   // 正常游戏：物理、管道、计分、碰撞
    GameOver,  // 结束：世界停止推进，只保留小鸟坠落动画，等待 R 重开
};

enum class Align { Left, Center, Right };

}  // namespace flappy
