#pragma once

#include <wx/window.h>

#ifdef __WXMSW__
#include <windows.h>
#endif

// ZH-Studio: rounded, modern window frames. On Windows 11 the system can round the
// corners of a window and draw its border in a custom colour; the DWM entry point is
// resolved at runtime so the build does not need to link dwmapi.
namespace ZHRounded {
    inline void Apply(wxWindow* window, int corner_preference = 2 /* DWMWCP_ROUND */,
                      unsigned long border_colour = 0)
    {
#ifdef __WXMSW__
        if (!window) return;
        HWND hwnd = window->GetHandle();
        if (!hwnd) return;

        typedef HRESULT (WINAPI *DwmSetWindowAttributeFn)(HWND, DWORD, LPCVOID, DWORD);
        static DwmSetWindowAttributeFn set_attribute = nullptr;
        static bool resolved = false;
        if (!resolved) {
            resolved = true;
            HMODULE dwm = LoadLibraryW(L"dwmapi.dll");
            if (dwm)
                set_attribute = reinterpret_cast<DwmSetWindowAttributeFn>(
                    reinterpret_cast<void*>(GetProcAddress(dwm, "DwmSetWindowAttribute")));
        }
        if (!set_attribute) return;

        const int pref = corner_preference;
        set_attribute(hwnd, 33 /* DWMWA_WINDOW_CORNER_PREFERENCE */, &pref, sizeof(pref));

        if (border_colour != 0) {
            const COLORREF colour = static_cast<COLORREF>(border_colour);
            set_attribute(hwnd, 34 /* DWMWA_BORDER_COLOR */, &colour, sizeof(colour));
        }
#else
        (void)window;
        (void)corner_preference;
        (void)border_colour;
#endif
    }
}
