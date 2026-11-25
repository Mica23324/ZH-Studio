#pragma once
#include <cmath>
#include <initializer_list>
#include <stdexcept>
#include <optional>
#include <string>

// UI numbers accept either decimal comma or decimal point, but no thousands
// separators, exponents, NaN/Inf or partially parsed text. Never silently clamp.
namespace ZHStudio {
inline std::optional<double> parse_decimal(const std::string& input, double maximum)
{
    if (input.size() > 64 || !std::isfinite(maximum) || maximum < 0) return std::nullopt;
    const auto begin = input.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) return std::nullopt;
    const auto end = input.find_last_not_of(" \t\r\n");
    bool separator = false, digit = false, fraction_digit = false;
    double value = 0.0, place = 0.1;
    for (size_t i = begin; i <= end; ++i) {
        const char c = input[i];
        if (c == '.' || c == ',') {
            if (separator) return std::nullopt;
            separator = true;
        } else if (c >= '0' && c <= '9') {
            digit = true;
            if (separator) { value += (c - '0') * place; place *= 0.1; fraction_digit = true; }
            else value = value * 10.0 + (c - '0');
            if (!std::isfinite(value) || value > maximum) return std::nullopt;
        } else return std::nullopt;
    }
    if (!digit || (separator && !fraction_digit)) return std::nullopt;
    return value;
}
}

namespace ZHStudio {
struct CostInput {
    double grams = 0.0;
    double hours = 0.0;
    double filament_per_kg = 25.0;
    double watts = 150.0;
    double electricity_per_kwh = 0.35;
    double wear_per_hour = 0.10;
};
struct CostResult {
    double material;
    double energy;
    double wear;
    double total;
    double kwh;
};
inline CostResult estimate_cost(const CostInput& in)
{
    for (double value : {in.grams, in.hours, in.filament_per_kg, in.watts, in.electricity_per_kwh, in.wear_per_hour})
        if (!std::isfinite(value) || value < 0.0)
            throw std::invalid_argument("Costs require finite, non-negative inputs");
    CostResult out{};
    out.material = in.grams / 1000.0 * in.filament_per_kg;
    out.kwh = in.watts / 1000.0 * in.hours;
    out.energy = out.kwh * in.electricity_per_kwh;
    out.wear = in.wear_per_hour * in.hours;
    out.total = out.material + out.energy + out.wear;
    if (!std::isfinite(out.total) || !std::isfinite(out.kwh))
        throw std::overflow_error("Cost estimate overflow");
    return out;
}
} // namespace ZHStudio
