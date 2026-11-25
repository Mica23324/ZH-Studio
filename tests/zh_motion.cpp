#include "../src/slic3r/GUI/ZHMotionMath.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

int main()
{
    using namespace ZHMotion;
    require(advance(0.f, 1.f, 0.25f) == 0.25f, "animation advances");
    require(advance(0.25f, 0.f, 0.1f) < 0.25f, "animation reverses");
    require(advance(0.25f, 1.f, 2.f) == 1.f, "long frame reaches target without overshoot");
    require(advance(0.25f, 0.f, 2.f) == 0.f, "reverse reaches target without overshoot");
    require(advance(0.25f, 1.f, -1.f) == 0.25f, "negative elapsed time is ignored");
    require(ease(-1.f) == 0.f && ease(2.f) == 1.f, "easing clamps to endpoints");
    require(std::abs(ease(0.5f) - 0.5f) < 0.0001f, "easing midpoint");
    require(blend_channel(0, 200, 0.f) == 0 && blend_channel(0, 200, 1.f) == 200, "colour endpoints");
    require(blend_channel(200, 0, 0.5f) == 100, "descending colour midpoint");
    require(ease_out(0.f) == 0.f && ease_out(1.f) == 1.f, "ease-out clamps to endpoints");
    require(ease_out(-3.f) == 0.f && ease_out(3.f) == 1.f, "ease-out clamps out-of-range input");
    require(ease_out(0.5f) > 0.5f, "ease-out front-loads the transition");
    require(ease_in_out(0.25f) < 0.25f && ease_in_out(0.75f) > 0.75f, "ease-in-out is symmetric");
    float previous = 0.f;
    for (int i = 0; i <= 32; ++i) {
        const float value = ease_out(static_cast<float>(i) / 32.f);
        require(value >= previous, "ease-out is monotonic");
        previous = value;
    }
    require(blend_channel(10, 20, 0.5f) == 15, "ascending colour midpoint");
    std::cout << "ZH motion tests passed.\n";
}
