#pragma once

namespace nina {

constexpr ExpressionPreset kIdlePreset = {
    ExpressionName::Idle,
    {{0x000000}, {0x0000EE}, {0x0000EE}, {0x0000EE}, {0x0000EE}},
    {36, 42, 0, 0, 42, 14, 0}
};

constexpr ExpressionPreset kCuriousPreset = {
    ExpressionName::Curious,
    {{0x000000}, {0x0000EE}, {0x0000EE}, {0x0000EE}, {0x0000EE}},
    {36, 46, 4, -2, 30, 8, 0}
};

constexpr ExpressionPreset kHappyPreset = {
    ExpressionName::Happy,
    {{0x000000}, {0x0000EE}, {0x0000EE}, {0x0000EE}, {0x0000EE}},
    {38, 38, 0, 1, 48, 20, 4}
};

constexpr ExpressionPreset kSurprisedPreset = {
    ExpressionName::Surprised,
    {{0x000000}, {0x0000EE}, {0x0000EE}, {0x0000EE}, {0x0000EE}},
    {38, 58, 0, 0, 28, 32, 10}
};

constexpr ExpressionPreset kExcitedPreset = {
    ExpressionName::Excited,
    {{0x000000}, {0x0000EE}, {0x0000EE}, {0x0000EE}, {0x0000EE}},
    {40, 48, 0, -1, 52, 26, 12}
};

inline const ExpressionPreset& expressionPreset(ExpressionName name) {
  switch (name) {
    case ExpressionName::Idle: return kIdlePreset;
    case ExpressionName::Curious: return kCuriousPreset;
    case ExpressionName::Happy: return kHappyPreset;
    case ExpressionName::Surprised: return kSurprisedPreset;
    case ExpressionName::Excited: return kExcitedPreset;
  }
  return kIdlePreset;
}

inline const ExpressionPreset& idlePreset() { return kIdlePreset; }

}  // namespace nina
