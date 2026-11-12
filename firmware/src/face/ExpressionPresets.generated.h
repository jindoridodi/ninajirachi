#pragma once

namespace nina {

constexpr ExpressionPreset kIdlePreset = {
    ExpressionName::Idle,
    {{0x000000}, {0xFFFFFF}, {0x000000}, {0x000000}},
    {36, 42, 0, 0, 42, 14, 0}
};

constexpr ExpressionPreset kCuriousPreset = {
    ExpressionName::Curious,
    {{0x000000}, {0xFFFFFF}, {0x000000}, {0x000000}},
    {36, 46, 4, -2, 30, 8, 0}
};

constexpr ExpressionPreset kExcitedPreset = {
    ExpressionName::Excited,
    {{0x000000}, {0xFFFFFF}, {0x000000}, {0x000000}},
    {40, 48, 0, -1, 52, 26, 0}
};

inline const ExpressionPreset& expressionPreset(ExpressionName name) {
  switch (name) {
    case ExpressionName::Idle: return kIdlePreset;
    case ExpressionName::Curious: return kCuriousPreset;
    case ExpressionName::Excited: return kExcitedPreset;
  }
  return kIdlePreset;
}

inline const ExpressionPreset& idlePreset() { return kIdlePreset; }

}  // namespace nina
