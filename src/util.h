// =============================================================================
//  util.h —— 小工具
// =============================================================================
#pragma once

#include <cstdlib>

namespace flappy {

// 闭区间 [lo, hi] 的随机整数，由 Game 构造里的 srand(time(nullptr)) 播种
inline int randomInt(int lo, int hi) {
    if (hi <= lo) return lo;
    return lo + std::rand() % (hi - lo + 1);
}

}  // namespace flappy
