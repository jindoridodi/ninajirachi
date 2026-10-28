#pragma once

namespace nina {

constexpr ExpressionPreset kIdlePreset = {
    ExpressionName::Idle,
    {0x21325E, 0xF6B3D1, 0xFFF8FC, 0xF6B3D1, 0x1D1630, 0x7B3157, 0xFFD45C},
    {112, 36, 42, 3, 10, 0, 0, 42, 14, 0}
};

inline const ExpressionPreset& idlePreset() { return kIdlePreset; }

}  // namespace nina
