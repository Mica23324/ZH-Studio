#pragma once

#include <wx/colour.h>
#include <atomic>
#include <string>

// ZH-Studio accent palettes. Persist the palette ID, not a translated display name.
// Startup selection happens before windows are constructed. Changing it requires a restart,
// because upstream caches bitmaps and several widgets keep static colour instances.
namespace ZHTheme {
inline std::atomic<int> palette_index{0};
inline constexpr const char* ids[] = {"cyan", "ocean", "violet", "amber"};
inline constexpr const char* normal[] = {"#0EA5E9", "#2563EB", "#7C3AED", "#D97706"};
inline constexpr const char* hover[] = {"#38BDF8", "#3B82F6", "#8B5CF6", "#F59E0B"};
inline constexpr const char* pressed[] = {"#0284C7", "#1D4ED8", "#6D28D9", "#B45309"};
inline constexpr const char* tints[] = {"#E0F2FE", "#DBEAFE", "#EDE9FE", "#FEF3C7"};
inline constexpr const char* dark_tints[] = {"#152E43", "#172B4D", "#2D2148", "#3B2C16"};

inline void select(const std::string& id)
{
    int selected = 0;
    for (int i = 0; i < 4; ++i)
        if (id == ids[i]) selected = i;
    palette_index.store(selected);
}
inline int index() { return palette_index.load(); }
inline wxColour accent() { return wxColour(normal[index()]); }
inline wxColour hovered() { return wxColour(hover[index()]); }
inline wxColour down() { return wxColour(pressed[index()]); }
inline wxColour tint() { return wxColour(tints[index()]); }
inline wxColour dark_tint() { return wxColour(dark_tints[index()]); }
inline std::string hex() { return normal[index()]; }

// Also catches colours cached before GUI_App loads the saved palette.
inline wxColour map(const wxColour& c)
{
    if (!c.IsOk()) return c;
    const unsigned r = c.Red(), g = c.Green(), b = c.Blue();
    if ((r == 0 && g == 174 && b == 66) || (r == 14 && g == 165 && b == 233)) return accent();
    if ((r == 61 && g == 203 && b == 115) || (r == 56 && g == 189 && b == 248)) return hovered();
    if ((r == 27 && g == 136 && b == 68) || (r == 2 && g == 132 && b == 199)) return down();
    if ((r == 237 && g == 250 && b == 242) || (r == 224 && g == 242 && b == 254)) return tint();
    return c;
}
} // namespace ZHTheme
