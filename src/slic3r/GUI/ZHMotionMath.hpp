#pragma once

#include <algorithm>
#include <cmath>

namespace ZHMotion {
inline float advance(float value, float target, float step)
{
    step = std::max(0.f, step);
    return value < target ? std::min(value + step, target) : std::max(value - step, target);
}

inline float ease(float amount)
{
    amount = std::max(0.f, std::min(1.f, amount));
    return amount * amount * (3.f - 2.f * amount);
}

inline unsigned char blend_channel(unsigned char from, unsigned char to, float amount)
{
    return static_cast<unsigned char>(std::lround(from + (static_cast<int>(to) - from) * ease(amount)));
}

inline float ease_out(float amount)
{
    amount = std::max(0.f, std::min(1.f, amount));
    return 1.f - std::pow(1.f - amount, 3.f);
}

inline float ease_in_out(float amount)
{
    amount = std::max(0.f, std::min(1.f, amount));
    return amount < 0.5f ? 2.f * amount * amount : 1.f - std::pow(-2.f * amount + 2.f, 3.f) / 2.f;
}
}
