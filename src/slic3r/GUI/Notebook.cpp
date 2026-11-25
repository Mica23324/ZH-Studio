#include "ZHTheme.hpp"
#include "ZHStyle.hpp"
#include "ZHMotion.hpp"
#include "Notebook.hpp"

//#ifdef _WIN32

#include "GUI_App.hpp"
#include "wxExtensions.hpp"
#include "Widgets/Button.hpp"
#include "libslic3r/Log.hpp"

//BBS set font size
#include "Widgets/Label.hpp"

#include <wx/button.h>
#include <wx/dcbuffer.h>
#include <wx/sizer.h>
#include <cmath>

namespace {
constexpr int btn_width_icon      = 40;
constexpr int btn_width_label_min = 48;
constexpr int btn_width_label_max = 164;
constexpr int btn_height          = 38;

StateColor tab_text(bool selected, bool dark)
{
    const wxColour color = selected ? (dark ? wxColour("#F8FAFC") : ZHTheme::accent()) :
                                      (dark ? ZHStyle::muted() : ZHStyle::secondary_text(false));
    return StateColor(std::pair{color, (int)StateColor::Normal});
}
}; // namespace

wxDEFINE_EVENT(wxCUSTOMEVT_NOTEBOOK_SEL_CHANGED, wxCommandEvent);

ButtonsListCtrl::ButtonsListCtrl(wxWindow *parent, wxBoxSizer* side_tools) :
    wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxTAB_TRAVERSAL),
    m_animation_timer(this)
{
#ifdef __WINDOWS__
    SetDoubleBuffered(true);
#endif //__WINDOWS__

    const wxColour default_btn_bg = Slic3r::GUI::wxGetApp().dark_mode() ? wxColour("#101E31") : wxColour("#F3F6FA");

   
    SetBackgroundColour(default_btn_bg);

    m_btn_margin = FromDIP(4);
    m_line_margin = FromDIP(3);

    m_sizer = new wxBoxSizer(wxHORIZONTAL);
    this->SetSizer(m_sizer);

    // a horizontal box sizer (instead of a flex grid) so the tab buttons can
    // shrink Chrome-style when the window is too narrow
    m_buttons_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto* tabs_column = new wxBoxSizer(wxVERTICAL);
    tabs_column->Add(m_buttons_sizer, 1, wxEXPAND);
    tabs_column->AddSpacer(FromDIP(5));
    m_sizer->Add(tabs_column, 1, wxEXPAND | wxLEFT | wxTOP, m_btn_margin);

    if (side_tools != NULL) {
        for (size_t idx = 0; idx < side_tools->GetItemCount(); idx++) {
            wxSizerItem* item = side_tools->GetItem(idx);
            wxWindow* item_win = item->GetWindow();
            if (item_win) {
                item_win->Reparent(this);
            }
        }
        m_sizer->Add(side_tools, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxBOTTOM, m_btn_margin);
    }

    SetBackgroundStyle(wxBG_STYLE_PAINT);
    Bind(wxEVT_PAINT, &ButtonsListCtrl::OnPaint, this);
    Bind(wxEVT_TIMER, &ButtonsListCtrl::on_animation, this, m_animation_timer.GetId());
    Bind(wxEVT_SIZE, [this](wxSizeEvent& e){
        m_marker_valid = false;
        Refresh(false);
        e.Skip();
    });
}

ButtonsListCtrl::~ButtonsListCtrl()
{
    m_animation_timer.Stop();
    for (Button* button : m_pageButtons) {
        button->Unbind(wxEVT_ENTER_WINDOW, &ButtonsListCtrl::on_tab_enter, this);
        button->Unbind(wxEVT_LEAVE_WINDOW, &ButtonsListCtrl::on_tab_leave, this);
    }
}

void ButtonsListCtrl::on_tab_enter(wxMouseEvent& event)
{
    m_hovered_button = dynamic_cast<Button*>(event.GetEventObject());
    start_animation();
    event.Skip();
}

void ButtonsListCtrl::on_tab_leave(wxMouseEvent& event)
{
    if (m_hovered_button == event.GetEventObject()) m_hovered_button = nullptr;
    start_animation();
    event.Skip();
}

void ButtonsListCtrl::OnPaint(wxPaintEvent&)
{
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(wxBrush(GetBackgroundColour()));
    dc.Clear();
    if (m_selection < 0 || m_selection >= static_cast<int>(m_pageButtons.size()))
        return;
    const wxRect rect = m_pageButtons[m_selection]->GetRect();
    if (!m_marker_valid && rect.width > 0) {
        m_marker_x = static_cast<float>(rect.x);
        m_marker_width = static_cast<float>(rect.width);
        m_marker_valid = true;
    }
    if (m_marker_valid) {
        const int inset = FromDIP(10);
        const int width = std::max(1, static_cast<int>(std::lround(m_marker_width)) - 2 * inset);
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.SetBrush(wxBrush(ZHTheme::accent()));
        dc.DrawRoundedRectangle(static_cast<int>(std::lround(m_marker_x)) + inset,
                                GetClientSize().y - m_line_margin - FromDIP(1), width, m_line_margin,
                                std::max(1, m_line_margin / 2));
    }
}

void ButtonsListCtrl::update_tab_style(size_t index, bool dark)
{
    const bool selected = static_cast<int>(index) == m_selection;
    const wxColour base = dark ? wxColour("#101E31") : ZHStyle::workspace_surface(false);
    const wxColour active = dark ? ZHTheme::dark_tint() : ZHTheme::tint();
    const wxColour hover = dark ? wxColour("#20324A") : wxColour("#E8EEF6");
    const wxColour selected_hover = dark ? wxColour("#204464") : ZHTheme::tint();
    const TabMotion& motion = m_tab_motion[index];
    const wxColour normal_color = ZHMotion::blend(base, active, motion.selection);
    const wxColour hover_color = ZHMotion::blend(hover, selected_hover, motion.selection);
    Button* button = m_pageButtons[index];
    button->SetBackgroundColor(ZHMotion::blend(normal_color, hover_color, motion.hover));
    button->SetBorderColor(StateColor(
        std::pair{ZHTheme::accent(), (int)StateColor::Focused},
        std::pair{selected ? ZHTheme::accent() : base, (int)StateColor::Normal}));
    button->SetTextColor(tab_text(selected, dark));
    button->SetSelected(selected);
}

void ButtonsListCtrl::snap_animation()
{
    m_animation_timer.Stop();
    const bool dark = Slic3r::GUI::wxGetApp().dark_mode();
    for (size_t i = 0; i < m_pageButtons.size(); ++i) {
        m_tab_motion[i].selection = static_cast<int>(i) == m_selection ? 1.f : 0.f;
        m_tab_motion[i].hover = m_pageButtons[i] == m_hovered_button ? 1.f : 0.f;
        update_tab_style(i, dark);
    }
    m_marker_valid = false;
    Refresh(false);
}

void ButtonsListCtrl::start_animation()
{
    m_animation_enabled = ZHMotion::enabled();
    if (!m_animation_enabled || !IsShownOnScreen()) {
        snap_animation();
        return;
    }
    if (!m_animation_timer.IsRunning()) {
        m_last_animation_tick = std::chrono::steady_clock::now();
        if (!m_animation_timer.Start(16)) {
            BOOST_LOG_TRIVIAL(warning) << "Could not start tab animation timer";
            snap_animation();
        }
    }
}

void ButtonsListCtrl::on_animation(wxTimerEvent&)
{
    if (!IsShownOnScreen() || !m_animation_enabled) {
        snap_animation();
        return;
    }
    const auto now = std::chrono::steady_clock::now();
    const float elapsed = std::chrono::duration<float>(now - m_last_animation_tick).count();
    m_last_animation_tick = now;
    const float step = std::max(0.f, elapsed / 0.16f);
    const bool dark = Slic3r::GUI::wxGetApp().dark_mode();
    bool moving = false;
    for (size_t i = 0; i < m_pageButtons.size(); ++i) {
        TabMotion& motion = m_tab_motion[i];
        const float selection = static_cast<int>(i) == m_selection ? 1.f : 0.f;
        const float hover = m_pageButtons[i] == m_hovered_button ? 1.f : 0.f;
        const float old_selection = motion.selection;
        const float old_hover = motion.hover;
        motion.selection = ZHMotion::advance(motion.selection, selection, step);
        motion.hover = ZHMotion::advance(motion.hover, hover, step);
        if (old_selection != motion.selection || old_hover != motion.hover)
            update_tab_style(i, dark);
        moving |= motion.selection != selection || motion.hover != hover;
    }
    bool marker_moved = false;
    float old_marker_x = 0.f;
    float old_marker_width = 0.f;
    if (m_selection >= 0 && m_selection < static_cast<int>(m_pageButtons.size())) {
        const wxRect target = m_pageButtons[m_selection]->GetRect();
        if (m_marker_valid && target.width > 0) {
            const float previous_marker_x = m_marker_x;
            const float previous_marker_width = m_marker_width;
            old_marker_x = previous_marker_x;
            old_marker_width = previous_marker_width;
            const float factor = std::min(1.f, std::max(0.f, elapsed / 0.07f));
            m_marker_x += (target.x - m_marker_x) * factor;
            m_marker_width += (target.width - m_marker_width) * factor;
            if (std::abs(m_marker_x - target.x) < 0.5f && std::abs(m_marker_width - target.width) < 0.5f) {
                m_marker_x = static_cast<float>(target.x);
                m_marker_width = static_cast<float>(target.width);
            } else {
                moving = true;
            }
            marker_moved = previous_marker_x != m_marker_x || previous_marker_width != m_marker_width;
        }
    }
    if (marker_moved) {
        const int inset = FromDIP(10);
        const int y = GetClientSize().y - m_line_margin - FromDIP(1);
        auto marker_rect = [&](float x, float width) {
            return wxRect(static_cast<int>(std::lround(x)) + inset, y,
                          std::max(1, static_cast<int>(std::lround(width)) - 2 * inset), m_line_margin);
        };
        RefreshRect(marker_rect(old_marker_x, old_marker_width).Union(marker_rect(m_marker_x, m_marker_width)), false);
    } else if (!m_marker_valid) {
        Refresh(false);
    }
    if (!moving)
        m_animation_timer.Stop();
}

void ButtonsListCtrl::UpdateMode()
{
    //m_mode_sizer->SetMode(Slic3r::GUI::wxGetApp().get_mode());
}

void ButtonsListCtrl::Rescale()
{
    //m_mode_sizer->msw_rescale();
    int em = em_unit(this);
    const bool dark = Slic3r::GUI::wxGetApp().dark_mode();
    SetBackgroundColour(dark ? wxColour("#101E31") : ZHStyle::workspace_surface(false));
    m_line_margin = FromDIP(3);
    for (size_t i = 0; i < m_pageButtons.size(); ++i)
        update_tab_style(i, dark);
    for (Button* btn : m_pageButtons) {
        // BBS: keep the Chrome-style shrinkable range in sync with the DPI scale.
        if (btn->GetLabel().empty()) {
            btn->SetMinSize({btn_width_icon * em / 10, btn_height * em / 10});
        } else {
            btn->SetMinSize({btn_width_label_min * em / 10, btn_height * em / 10});
            btn->SetMaxSize({btn_width_label_max * em / 10, btn_height * em / 10});
        }
        btn->Rescale();
    }

    // BBS: no gap
    //m_btn_margin = std::lround(0.3 * em);
    //m_line_margin = std::lround(0.1 * em);
    //m_buttons_sizer->SetVGap(m_btn_margin);
    //m_buttons_sizer->SetHGap(m_btn_margin);

    m_sizer->Layout();
    m_marker_valid = false;
    Refresh(false);
}

void ButtonsListCtrl::SetSelection(int sel)
{
    if (sel < 0 || sel >= static_cast<int>(m_pageButtons.size()) || m_selection == sel)
        return;
    const int previous = m_selection;
    const bool dark = Slic3r::GUI::wxGetApp().dark_mode();
    m_selection = sel;
    if (previous >= 0 && previous < static_cast<int>(m_pageButtons.size()))
        update_tab_style(previous, dark);
    update_tab_style(sel, dark);
    if (previous < 0)
        snap_animation();
    else
        start_animation();
}

bool ButtonsListCtrl::InsertPage(size_t n, const wxString &text, bool bSelect /* = false*/, const std::string &bmp_name /* = ""*/, const std::string &inactive_bmp_name)
{
    if (n > m_pageButtons.size()) return false;
    Button * btn = new Button(this, text.empty() ? text : " " + text, bmp_name, wxNO_BORDER);
    btn->SetCornerRadius(FromDIP(7));
    const bool dark = Slic3r::GUI::wxGetApp().dark_mode();
    btn->SetBorderColor(dark ? wxColour("#101E31") : ZHStyle::workspace_surface(false));

    // always show the tab name as a tooltip so the user can identify a tab
    // by hovering even when it is shrunk and the label is truncated with an ellipsis.
    // Icon-only tabs (empty label) get their tooltip set by the caller via Notebook::SetPageToolTip.
    if (!text.empty()) btn->SetToolTip(text);

    int em = em_unit(this);
    // BBS set size for button
    //  Chrome-style labeled tabs may shrink down to a small floor when crowded and
    //  grow up to the preferred width (136) when there is room, so the side tools
    //  (slice/print) on the right always stay visible. Icon-only tabs keep a fixed size.
    if (text.empty()) {
        btn->SetMinSize({btn_width_icon * em / 10, btn_height * em / 10});
    } else {
        btn->SetAllowShrink(true);
        btn->SetMinSize({btn_width_label_min * em / 10, btn_height * em / 10});
        btn->SetMaxSize({btn_width_label_max * em / 10, btn_height * em / 10});
    }

    btn->SetBackgroundColor(dark ? wxColour("#101E31") : ZHStyle::workspace_surface(false));
    StateColor text_color = tab_text(false, dark);
    btn->SetTextColor(text_color);
    btn->SetInactiveIcon(inactive_bmp_name);
    btn->SetSelected(false);
    btn->Bind(wxEVT_ENTER_WINDOW, &ButtonsListCtrl::on_tab_enter, this);
    btn->Bind(wxEVT_LEAVE_WINDOW, &ButtonsListCtrl::on_tab_leave, this);
    btn->Bind(wxEVT_BUTTON, [this, btn](wxCommandEvent& event) {
        if (auto it = std::find(m_pageButtons.begin(), m_pageButtons.end(), btn); it != m_pageButtons.end()) {
            auto sel = it - m_pageButtons.begin();
            //do it later
            //SetSelection(sel);
            
            wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_NOTEBOOK_SEL_CHANGED);
            evt.SetId(sel);
            wxPostEvent(this->GetParent(), evt);
        }
    });
    Slic3r::GUI::wxGetApp().UpdateDarkUI(btn);
    m_pageButtons.insert(m_pageButtons.begin() + n, btn);
    m_tab_motion.insert(m_tab_motion.begin() + n, TabMotion{});
    if (m_selection >= static_cast<int>(n)) ++m_selection;
    // Labeled tabs get proportion 1 so they share the tab region equally (Chrome style)
    // Icon-only stay fixed (proportion 0) at their own min size
    wxSizerFlags flags = text.empty() ? wxSizerFlags(0) : wxSizerFlags(1);
    m_buttons_sizer->Insert(n, btn, flags.Align(wxALIGN_CENTER_VERTICAL).Border(wxRIGHT, FromDIP(2)));
    m_sizer->Layout();
    m_marker_valid = false;
    Refresh(false);
    return true;
}

void ButtonsListCtrl::RemovePage(size_t n)
{
    if (n >= m_pageButtons.size()) return;
    const bool selected_removed = m_selection == static_cast<int>(n);
    Button* btn = m_pageButtons[n];
    btn->Unbind(wxEVT_ENTER_WINDOW, &ButtonsListCtrl::on_tab_enter, this);
    btn->Unbind(wxEVT_LEAVE_WINDOW, &ButtonsListCtrl::on_tab_leave, this);
    if (m_hovered_button == btn) m_hovered_button = nullptr;
    m_pageButtons.erase(m_pageButtons.begin() + n);
    m_tab_motion.erase(m_tab_motion.begin() + n);
    if (selected_removed) m_selection = -1;
    else if (m_selection > static_cast<int>(n)) --m_selection;
    if (selected_removed && !m_pageButtons.empty())
        SetSelection(static_cast<int>(std::min(n, m_pageButtons.size() - 1)));
    m_buttons_sizer->Remove(n);
#if __WXOSX__
    RemoveChild(btn);
#else
    btn->Reparent(nullptr);
#endif
    btn->Destroy();
    m_sizer->Layout();
    m_marker_valid = false;
    Refresh(false);
}

bool ButtonsListCtrl::SetPageImage(size_t n, const std::string& bmp_name) const
{
    if (n >= m_pageButtons.size())
        return false;
     
    // BBS
    //return m_pageButtons[n]->SetBitmap_(bmp_name);
    m_pageButtons[n]->SetIcon(wxString::FromUTF8(bmp_name));
    return true;
}

void ButtonsListCtrl::SetPageText(size_t n, const wxString& strText)
{
    if (n >= m_pageButtons.size()) return;
    Button* btn = m_pageButtons[n];
    btn->SetLabel(strText);
    // keep the hover tooltip in sync with the (possibly truncated) label
    if (!strText.empty()) btn->SetToolTip(strText);
    m_sizer->Layout();
    m_marker_valid = false;
    Refresh(false);
}

void ButtonsListCtrl::SetPageToolTip(size_t n, const wxString &strToolTip)
{
    if (n >= m_pageButtons.size()) return;
    m_pageButtons[n]->SetToolTip(strToolTip);
}

wxString ButtonsListCtrl::GetPageText(size_t n) const
{
    if (n >= m_pageButtons.size()) return wxEmptyString;
    Button* btn = m_pageButtons[n];
    return btn->GetLabel();
}

//#endif // _WIN32

void Notebook::Init()
{
    // We don't need any border as we don't have anything to separate the
    // page contents from.
    SetInternalBorder(0);

    // No effects by default.
    m_showEffect = m_hideEffect = wxSHOW_EFFECT_NONE;

    m_showTimeout = m_hideTimeout = 0;

    /* On Linux, Gstreamer wxMediaCtrl does not seem to get along well with
     * 32-bit X11 visuals (the overlay does not work).  Is this a wxWindows
     * bug?  Is this a Gstreamer bug?  No idea, but it is our problem ... 
     * and anyway, this transparency thing just isn't all that interesting,
     * so we just don't do it on Linux. 
     */
#ifndef __WXGTK__
    SetBackgroundStyle(wxBG_STYLE_TRANSPARENT);
#endif
}
