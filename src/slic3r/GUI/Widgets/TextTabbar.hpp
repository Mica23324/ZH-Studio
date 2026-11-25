#ifndef slic3r_GUI_TextTabbar_hpp_
#define slic3r_GUI_TextTabbar_hpp_

#include <wx/control.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/timer.h>
#include <chrono>
#include <vector>

namespace Slic3r { namespace GUI {

// TextTabbar - plain-text top tabs
// a horizontal row of labels with a green underline under the selected tab,
// over a 1px divider line. Emits the standard wxEVT_CHOICE (int = selected index)
// when the user clicks a tab.
//
// Alignment: Center spreads tabs evenly across the full width (Preferences look);
// Left packs tabs to the left with a fixed gap between them (preset-compare look).
//
// tab_gap (Left align only) is the fixed spacing between adjacent tabs, in unscaled
// DIP; pass a value to widen/tighten it. It is read when tabs are added, so set it
// (via the ctor) before AddTab()/ClearTabs()+rebuild.
class TextTabbar : public wxControl
{
public:
    enum class Align { Center, Left };

    TextTabbar(wxWindow *parent, Align align = Align::Center, int tab_gap = 24);
    ~TextTabbar() override;
    void AddTab(const wxString &label);
    void ClearTabs();
    void SetSelection(int sel);
    int  GetSelection() const { return m_selection; }
    void Rescale();

private:
    void                        render();
    void                        update_colors();
    void                        start_animation();
    void                        snap_animation();
    void                        on_animation(wxTimerEvent& event);
    void                        on_tab_enter(wxMouseEvent& event);
    void                        on_tab_leave(wxMouseEvent& event);
    Align                       m_align;
    int                         m_tab_gap;  // Left-align inter-tab spacing (DIP)
    std::vector<wxStaticText *> m_labels;
    std::vector<wxWindow *>     m_underlines;
    struct TabMotion { float selection = 0.f; float hover = 0.f; };
    std::vector<TabMotion>      m_tab_motion;
    wxWindow*                   m_hovered = nullptr;
    wxTimer                     m_animation_timer;
    std::chrono::steady_clock::time_point m_last_animation_tick;
    wxWindow*                   m_divider = nullptr;
    wxBoxSizer                 *m_row       = nullptr;
    int                         m_selection = -1;
};

}} // namespace Slic3r::GUI

#endif // slic3r_GUI_TextTabbar_hpp_
