#include "TextTabbar.hpp"

#include <wx/panel.h>

#include <algorithm>

#include "Label.hpp"
#include "StateColor.hpp"
#include "../ZHStyle.hpp"
#include "../ZHMotion.hpp"
#include "../GUI_App.hpp"
#include "libslic3r/Log.hpp"

namespace Slic3r { namespace GUI {

TextTabbar::TextTabbar(wxWindow *parent, Align align, int tab_gap)
    : wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE), m_align(align), m_tab_gap(tab_gap), m_animation_timer(this)
{
    // ZH-Studio: the tab strip lives on the workspace surface, not on white.
    SetBackgroundColour(ZHStyle::workspace_surface(wxGetApp().dark_mode()));
    auto *outer = new wxBoxSizer(wxVERTICAL);
    m_row       = new wxBoxSizer(wxHORIZONTAL);

    const int side_border = m_align == Align::Center ? FromDIP(48) : FromDIP(8);
    outer->Add(m_row, wxSizerFlags().Expand().Border(wxLEFT | wxRIGHT, side_border));
    m_divider = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 1));
    m_divider->SetBackgroundColour(ZHStyle::outline(wxGetApp().dark_mode()));
    outer->Add(m_divider, wxSizerFlags().Expand());

    SetSizer(outer);
    Bind(wxEVT_TIMER, &TextTabbar::on_animation, this, m_animation_timer.GetId());
}

TextTabbar::~TextTabbar()
{
    m_animation_timer.Stop();
    for (wxStaticText* text : m_labels) {
        text->Unbind(wxEVT_ENTER_WINDOW, &TextTabbar::on_tab_enter, this);
        text->Unbind(wxEVT_LEAVE_WINDOW, &TextTabbar::on_tab_leave, this);
    }
}

void TextTabbar::ClearTabs()
{
    m_animation_timer.Stop();
    m_hovered = nullptr;
    // Destroy the label/underline windows we own, then drop the (now dangling) sizer items.
    for (auto *w : m_labels) {
        w->Unbind(wxEVT_ENTER_WINDOW, &TextTabbar::on_tab_enter, this);
        w->Unbind(wxEVT_LEAVE_WINDOW, &TextTabbar::on_tab_leave, this);
        w->Destroy();
    }
    for (auto *w : m_underlines) {
        w->Unbind(wxEVT_ENTER_WINDOW, &TextTabbar::on_tab_enter, this);
        w->Unbind(wxEVT_LEAVE_WINDOW, &TextTabbar::on_tab_leave, this);
        w->Destroy();
    }
    m_labels.clear();
    m_underlines.clear();
    m_tab_motion.clear();
    m_row->Clear(/*delete_windows*/ false);
    m_selection = -1;
    Layout();
}

void TextTabbar::AddTab(const wxString &label)
{
    const int index = (int) m_labels.size();

    auto *col  = new wxBoxSizer(wxVERTICAL);
    auto *text = new wxStaticText(this, wxID_ANY, label);
    text->SetFont(::Label::Body_14);

    auto *underline = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, FromDIP(2)));
    underline->SetBackgroundColour(this->GetBackgroundColour());

    auto on_click = [this, index](wxMouseEvent &) {
        SetSelection(index);
        wxCommandEvent evt(wxEVT_CHOICE, GetId());
        evt.SetEventObject(this);
        evt.SetInt(index);
        wxPostEvent(this, evt);
    };

    text->Bind(wxEVT_LEFT_DOWN, on_click);
    underline->Bind(wxEVT_LEFT_DOWN, on_click);
    text->SetCursor(wxCURSOR_HAND);
    text->Bind(wxEVT_ENTER_WINDOW, &TextTabbar::on_tab_enter, this);
    text->Bind(wxEVT_LEAVE_WINDOW, &TextTabbar::on_tab_leave, this);
    underline->Bind(wxEVT_ENTER_WINDOW, &TextTabbar::on_tab_enter, this);
    underline->Bind(wxEVT_LEAVE_WINDOW, &TextTabbar::on_tab_leave, this);

    col->AddStretchSpacer();
    col->Add(text);
    col->AddStretchSpacer();
    col->Add(underline, 0, wxEXPAND);

    m_labels.push_back(text);
    m_underlines.push_back(underline);
    m_tab_motion.push_back(TabMotion{});

    if (m_align == Align::Center) {
        // Even spread: a stretch spacer between every pair of tabs.
        if (m_row->GetItemCount() != 0) m_row->AddStretchSpacer();
        m_row->Add(col, wxSizerFlags().CenterHorizontal());
    } else {
        // Left-packed: fixed gap between tabs, trailing stretch pushes them all left.
        if (m_row->GetItemCount() != 0) m_row->AddSpacer(FromDIP(std::max(0, m_tab_gap)));
        m_row->Add(col);
    }

    if (m_selection < 0) SetSelection(0);
    else update_colors();
}

void TextTabbar::SetSelection(int sel)
{
    if (sel < 0 || sel >= (int) m_labels.size() || sel == m_selection) return;

    const int previous = m_selection;
    m_selection = sel;
    render();
    if (previous < 0) snap_animation();
    else start_animation();
}

void TextTabbar::render()
{
    for (int i = 0; i < (int) m_labels.size(); ++i) {
        const bool active = (i == m_selection);
        m_labels[i]->SetFont(active ? Label::Head_14 : Label::Body_14);
    }
    update_colors();
    Layout();
    Refresh();
}

void TextTabbar::update_colors()
{
    const bool dark = wxGetApp().dark_mode();
    const wxColour normal = ZHStyle::secondary_text(dark);
    const wxColour active = dark ? ZHStyle::text() : ZHTheme::accent();
    for (size_t i = 0; i < m_labels.size(); ++i) {
        const TabMotion& motion = m_tab_motion[i];
        const wxColour text = ZHMotion::blend(normal, active, motion.selection);
        m_labels[i]->SetForegroundColour(ZHMotion::blend(text, active, motion.hover * 0.55f));
        m_underlines[i]->SetBackgroundColour(ZHMotion::blend(GetBackgroundColour(), ZHTheme::accent(), motion.selection));
        m_underlines[i]->Refresh(false);
    }
}

void TextTabbar::snap_animation()
{
    m_animation_timer.Stop();
    for (size_t i = 0; i < m_labels.size(); ++i) {
        m_tab_motion[i].selection = static_cast<int>(i) == m_selection ? 1.f : 0.f;
        m_tab_motion[i].hover = m_labels[i] == m_hovered ? 1.f : 0.f;
    }
    update_colors();
}

void TextTabbar::start_animation()
{
    if (!ZHMotion::enabled() || !IsShownOnScreen()) {
        snap_animation();
        return;
    }
    if (!m_animation_timer.IsRunning()) {
        m_last_animation_tick = std::chrono::steady_clock::now();
        if (!m_animation_timer.Start(16)) {
            BOOST_LOG_TRIVIAL(warning) << "Could not start text tab animation timer";
            snap_animation();
        }
    }
}

void TextTabbar::on_animation(wxTimerEvent&)
{
    if (!IsShownOnScreen() || !ZHMotion::enabled()) {
        snap_animation();
        return;
    }
    const auto now = std::chrono::steady_clock::now();
    const float step = std::chrono::duration<float>(now - m_last_animation_tick).count() / 0.14f;
    m_last_animation_tick = now;
    bool moving = false;
    for (size_t i = 0; i < m_labels.size(); ++i) {
        TabMotion& motion = m_tab_motion[i];
        const float selection = static_cast<int>(i) == m_selection ? 1.f : 0.f;
        const float hover = m_labels[i] == m_hovered ? 1.f : 0.f;
        const float old_selection = motion.selection;
        const float old_hover = motion.hover;
        motion.selection = ZHMotion::advance(motion.selection, selection, step);
        motion.hover = ZHMotion::advance(motion.hover, hover, step);
        if (old_selection != motion.selection || old_hover != motion.hover) moving = true;
        moving |= motion.selection != selection || motion.hover != hover;
    }
    update_colors();
    if (!moving) m_animation_timer.Stop();
}

void TextTabbar::on_tab_enter(wxMouseEvent& event)
{
    wxWindow* target = dynamic_cast<wxWindow*>(event.GetEventObject());
    for (size_t i = 0; i < m_labels.size(); ++i)
        if (target == m_labels[i] || target == m_underlines[i]) {
            m_hovered = m_labels[i];
            break;
        }
    start_animation();
    event.Skip();
}

void TextTabbar::on_tab_leave(wxMouseEvent& event)
{
    wxWindow* target = dynamic_cast<wxWindow*>(event.GetEventObject());
    for (size_t i = 0; i < m_labels.size(); ++i)
        if ((target == m_labels[i] || target == m_underlines[i]) && m_hovered == m_labels[i]) {
            m_hovered = nullptr;
            break;
        }
    start_animation();
    event.Skip();
}

void TextTabbar::Rescale()
{
    const bool dark = wxGetApp().dark_mode();
    SetBackgroundColour(ZHStyle::workspace_surface(dark));
    m_divider->SetBackgroundColour(ZHStyle::outline(dark));
    render();
}

}} // namespace Slic3r::GUI
