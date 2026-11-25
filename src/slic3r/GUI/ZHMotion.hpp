#pragma once

#include "ZHMotionMath.hpp"
#include <wx/colour.h>

#ifdef __WXMSW__
#include <wx/msw/wrapwin.h>
#endif

namespace ZHMotion {
inline bool enabled()
{
#ifdef __WXMSW__
    BOOL value = TRUE;
    if (::SystemParametersInfo(SPI_GETCLIENTAREAANIMATION, 0, &value, 0))
        return value != FALSE;
#endif
    return true;
}

inline wxColour blend(const wxColour& from, const wxColour& to, float amount)
{
    return wxColour(blend_channel(from.Red(), to.Red(), amount),
                    blend_channel(from.Green(), to.Green(), amount),
                    blend_channel(from.Blue(), to.Blue(), amount));
}
}
