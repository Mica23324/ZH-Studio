#include "Tabbook.hpp"

//#ifdef _WIN32

#include "GUI_App.hpp"
#include "wxExtensions.hpp"
#include "TabButton.hpp"
#include "ZHMotion.hpp"
#include "ZHStyle.hpp"
#include "ZHTheme.hpp"
#include "libslic3r/Log.hpp"

//BBS set font size
#include "Widgets/Label.hpp"

#include <wx/button.h>
#include <wx/dcbuffer.h>
#include <wx/sizer.h>

wxDEFINE_EVENT(wxCUSTOMEVT_TABBOOK_SEL_CHANGED, wxCommandEvent);

static const wxFont& TAB_BUTTON_FONT     = Label::Body_14;
static const wxFont& TAB_BUTTON_FONT_SEL = Label::Head_14;


static const int BUTTON_DEF_HEIGHT = 46;
static const int BUTTON_DEF_WIDTH  = 220;


TabButtonsListCtrl::TabButtonsListCtrl(wxWindow *parent, wxBoxSizer *side_tools) :
    wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxTAB_TRAVERSAL),
    m_animation_timer(this)
{
#ifdef __WINDOWS__
    SetDoubleBuffered(true);
#endif //__WINDOWS__
    SetBackgroundColour(ZHStyle::workspace_surface(Slic3r::GUI::wxGetApp().dark_mode()));

    m_btn_margin = FromDIP(6);

    m_arrow_img = ScalableBitmap(this, "monitor_arrow", 14);

    m_sizer = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(m_sizer);
    if (side_tools != NULL) {
        for (size_t idx = 0; idx < side_tools->GetItemCount(); idx++) {
            wxSizerItem *item     = side_tools->GetItem(idx);
            wxWindow *   item_win = item->GetWindow();
            if (item_win) { item_win->Reparent(this); }
        }
        m_sizer->Add(side_tools, 0, wxEXPAND | wxLEFT | wxTOP, m_btn_margin);
    }

    m_buttons_sizer = new wxFlexGridSizer(1, FromDIP(4), 0);
    m_sizer->Add(m_buttons_sizer, 0, wxLEFT | wxTOP, m_btn_margin);
    m_sizer->AddStretchSpacer(1);
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    Bind(wxEVT_PAINT, &TabButtonsListCtrl::OnPaint, this);
    Bind(wxEVT_SIZE, &TabButtonsListCtrl::on_size, this);
    Bind(wxEVT_TIMER, &TabButtonsListCtrl::on_animation, this, m_animation_timer.GetId());
}

TabButtonsListCtrl::~TabButtonsListCtrl()
{
    m_animation_timer.Stop();
    for (TabButton* button : m_pageButtons) {
        button->Unbind(wxEVT_ENTER_WINDOW, &TabButtonsListCtrl::on_tab_enter, this);
        button->Unbind(wxEVT_LEAVE_WINDOW, &TabButtonsListCtrl::on_tab_leave, this);
    }
}

void TabButtonsListCtrl::on_tab_enter(wxMouseEvent& event)
{
    m_hovered_button = dynamic_cast<TabButton*>(event.GetEventObject());
    start_animation();
    event.Skip();
}

void TabButtonsListCtrl::on_tab_leave(wxMouseEvent& event)
{
    if (m_hovered_button == event.GetEventObject()) m_hovered_button = nullptr;
    start_animation();
    event.Skip();
}

void TabButtonsListCtrl::update_tab_style(size_t index)
{
    const bool dark = Slic3r::GUI::wxGetApp().dark_mode();
    const bool selected = static_cast<int>(index) == m_selection;
    const wxColour base = ZHStyle::workspace_surface(dark);
    const wxColour active = dark ? ZHTheme::dark_tint() : ZHTheme::tint();
    const wxColour hover = dark ? ZHStyle::elev_hover() : wxColour("#E8EEF6");
    const wxColour selected_hover = dark ? wxColour("#204464") : ZHTheme::tint();
    const TabMotion& motion = m_tab_motion[index];
    TabButton* button = m_pageButtons[index];
    button->SetBackgroundColour(base);
    button->SetBGColor(ZHMotion::blend(ZHMotion::blend(base, active, motion.selection),
                                       ZHMotion::blend(hover, selected_hover, motion.selection), motion.hover));
    button->SetBorderColor(StateColor(std::pair{ZHTheme::accent(), (int)StateColor::Focused},
                                      std::pair{selected ? ZHTheme::accent() : base, (int)StateColor::Normal}));
    button->SetTextColor(selected ? ZHStyle::primary_text(dark) : ZHStyle::secondary_text(dark));
}

void TabButtonsListCtrl::snap_animation()
{
    m_animation_timer.Stop();
    for (size_t i = 0; i < m_pageButtons.size(); ++i) {
        m_tab_motion[i].selection = static_cast<int>(i) == m_selection ? 1.f : 0.f;
        m_tab_motion[i].hover = m_pageButtons[i] == m_hovered_button ? 1.f : 0.f;
        update_tab_style(i);
    }
    m_indicator_valid = false;
    Refresh(false);
}

void TabButtonsListCtrl::on_size(wxSizeEvent& event)
{
    if (m_selection >= 0 && m_selection < static_cast<int>(m_pageButtons.size())) {
        const wxRect target = m_pageButtons[m_selection]->GetRect();
        if (target.height > 0) {
            m_indicator_y = static_cast<float>(target.y);
            m_indicator_height = static_cast<float>(target.height);
            m_indicator_valid = true;
        }
    } else {
        m_indicator_valid = false;
    }
    Refresh(false);
    event.Skip();
}

void TabButtonsListCtrl::start_animation()
{
    if (!ZHMotion::enabled() || !IsShownOnScreen()) {
        snap_animation();
        return;
    }
    if (!m_animation_timer.IsRunning()) {
        m_last_animation_tick = std::chrono::steady_clock::now();
        if (!m_animation_timer.Start(16)) {
            BOOST_LOG_TRIVIAL(warning) << "Could not start sidebar tab animation timer";
            snap_animation();
        }
    }
}

void TabButtonsListCtrl::on_animation(wxTimerEvent&)
{
    if (!IsShownOnScreen() || !ZHMotion::enabled()) {
        snap_animation();
        return;
    }
    const auto now = std::chrono::steady_clock::now();
    const float elapsed = std::chrono::duration<float>(now - m_last_animation_tick).count();
    const float step = elapsed / 0.16f;
    m_last_animation_tick = now;
    bool moving = false;
    for (size_t i = 0; i < m_pageButtons.size(); ++i) {
        TabMotion& motion = m_tab_motion[i];
        const float selection = static_cast<int>(i) == m_selection ? 1.f : 0.f;
        const float hover = m_pageButtons[i] == m_hovered_button ? 1.f : 0.f;
        const float old_selection = motion.selection;
        const float old_hover = motion.hover;
        motion.selection = ZHMotion::advance(motion.selection, selection, step);
        motion.hover = ZHMotion::advance(motion.hover, hover, step);
        if (old_selection != motion.selection || old_hover != motion.hover) update_tab_style(i);
        moving |= motion.selection != selection || motion.hover != hover;
    }
    if (m_selection >= 0 && m_selection < static_cast<int>(m_pageButtons.size())) {
        const wxRect target = m_pageButtons[m_selection]->GetRect();
        if (!m_indicator_valid && target.height > 0) {
            m_indicator_y = static_cast<float>(target.y);
            m_indicator_height = static_cast<float>(target.height);
            m_indicator_valid = true;
        } else if (m_indicator_valid && target.height > 0) {
            const float factor = std::min(1.f, std::max(0.f, elapsed / 0.07f));
            const float dy = target.y - m_indicator_y;
            const float dh = target.height - m_indicator_height;
            m_indicator_y += dy * factor;
            m_indicator_height += dh * factor;
            if (std::abs(dy) < 0.5f && std::abs(dh) < 0.5f) {
                m_indicator_y = static_cast<float>(target.y);
                m_indicator_height = static_cast<float>(target.height);
            } else {
                moving = true;
            }
        }
    }
    Refresh(false);
    if (!moving) m_animation_timer.Stop();
}

void TabButtonsListCtrl::OnPaint(wxPaintEvent &)
{
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(wxBrush(GetBackgroundColour()));
    dc.Clear();
    if (m_selection < 0 || m_selection >= (int)m_pageButtons.size())
        return;
    const wxRect target = m_pageButtons[m_selection]->GetRect();
    if (!m_indicator_valid && target.height > 0) {
        m_indicator_y = static_cast<float>(target.y);
        m_indicator_height = static_cast<float>(target.height);
        m_indicator_valid = true;
    }
    if (m_indicator_valid) {
        const int y = static_cast<int>(std::lround(m_indicator_y)) + FromDIP(9);
        const int height = std::max(FromDIP(12), static_cast<int>(std::lround(m_indicator_height)) - FromDIP(18));
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.SetBrush(wxBrush(ZHTheme::accent()));
        dc.DrawRoundedRectangle(FromDIP(1), y, FromDIP(3), height, FromDIP(2));
    }
}

void TabButtonsListCtrl::Rescale()
{
    m_arrow_img = ScalableBitmap(this, "monitor_arrow", 14);
    SetBackgroundColour(ZHStyle::workspace_surface(Slic3r::GUI::wxGetApp().dark_mode()));
    m_buttons_sizer->SetVGap(FromDIP(4));

    int em = em_unit(this);
    for (size_t i = 0; i < m_pageButtons.size(); ++i) {
        TabButton* btn = m_pageButtons[i];
        btn->SetMinSize({BUTTON_DEF_WIDTH * em / 10, BUTTON_DEF_HEIGHT * em / 10});
        btn->SetCornerRadius(FromDIP(8));
        btn->SetBitmap(m_arrow_img);
        btn->Rescale();
        update_tab_style(i);
    }

    m_sizer->Layout();
    if (m_selection >= 0 && m_selection < static_cast<int>(m_pageButtons.size())) {
        const wxRect target = m_pageButtons[m_selection]->GetRect();
        m_indicator_y = static_cast<float>(target.y);
        m_indicator_height = static_cast<float>(target.height);
        m_indicator_valid = target.height > 0;
    }
    Refresh(false);
}

void TabButtonsListCtrl::SetSelection(int sel)
{
    if (sel < 0 || sel >= static_cast<int>(m_pageButtons.size()) || m_selection == sel)
        return;
    const int previous = m_selection;
    if (previous >= 0 && previous < static_cast<int>(m_pageButtons.size())) {
        m_pageButtons[previous]->SetFont(TAB_BUTTON_FONT);
    }
    m_selection = sel;
    m_pageButtons[m_selection]->SetFont(TAB_BUTTON_FONT_SEL);
    if (previous >= 0 && previous < static_cast<int>(m_pageButtons.size())) update_tab_style(previous);
    update_tab_style(sel);
    if (previous < 0) {
        snap_animation();
    } else {
        start_animation();
    }
}

void TabButtonsListCtrl::showNewTag(int sel, int count)
{
    if (sel < 0 || sel >= static_cast<int>(m_pageButtons.size()))
        return;
    if (m_pageButtons[sel]->GetNewTagCount() == count)
    {
        return;
    }

    m_pageButtons[sel]->ShowNewTag(count);
    Refresh();
}

bool TabButtonsListCtrl::InsertPage(size_t n, const wxString &text, bool bSelect /* = false*/, const std::string &bmp_name /* = ""*/)
{
    TabButton *btn = new TabButton(this, text, m_arrow_img, wxNO_BORDER);
    btn->SetCornerRadius(FromDIP(8));

    int em = em_unit(this);
    btn->SetMinSize({BUTTON_DEF_WIDTH * em / 10, BUTTON_DEF_HEIGHT * em / 10});

    const wxColour base = ZHStyle::workspace_surface(Slic3r::GUI::wxGetApp().dark_mode());
    btn->SetBackgroundColour(base);
    btn->SetBGColor(base);
    btn->SetTextColor(ZHStyle::secondary_text(Slic3r::GUI::wxGetApp().dark_mode()));
    btn->Bind(wxEVT_ENTER_WINDOW, &TabButtonsListCtrl::on_tab_enter, this);
    btn->Bind(wxEVT_LEAVE_WINDOW, &TabButtonsListCtrl::on_tab_leave, this);
    btn->Bind(wxEVT_BUTTON, [this, btn](wxCommandEvent& event) {
        if (auto it = std::find(m_pageButtons.begin(), m_pageButtons.end(), btn); it != m_pageButtons.end()) {
            auto sel = it - m_pageButtons.begin();
            wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_TABBOOK_SEL_CHANGED);
            evt.SetId(sel);
            wxPostEvent(this->GetParent(), evt);
        }
    });
    Slic3r::GUI::wxGetApp().UpdateDarkUI(btn);
    m_pageButtons.insert(m_pageButtons.begin() + n, btn);
    m_tab_motion.insert(m_tab_motion.begin() + n, TabMotion{});
    if (m_selection >= static_cast<int>(n)) ++m_selection;
    update_tab_style(n);
    m_buttons_sizer->Insert(n, new wxSizerItem(btn));
    m_buttons_sizer->SetRows(m_pageButtons.size() + 1);
    m_sizer->Layout();
    if (bSelect)
        SetSelection(static_cast<int>(n));
    else if (m_selection >= 0)
        update_tab_style(static_cast<size_t>(m_selection));
    m_indicator_valid = false;
    Refresh(false);
    return true;
}

void TabButtonsListCtrl::RemovePage(size_t n)
{
    if (n >= m_pageButtons.size()) return;
    TabButton *btn = m_pageButtons[n];
    btn->Unbind(wxEVT_ENTER_WINDOW, &TabButtonsListCtrl::on_tab_enter, this);
    btn->Unbind(wxEVT_LEAVE_WINDOW, &TabButtonsListCtrl::on_tab_leave, this);
    if (m_hovered_button == btn) m_hovered_button = nullptr;
    m_pageButtons.erase(m_pageButtons.begin() + n);
    m_tab_motion.erase(m_tab_motion.begin() + n);
    const bool removed_selection = m_selection == static_cast<int>(n);
    if (removed_selection) m_selection = -1;
    else if (m_selection > static_cast<int>(n)) --m_selection;
    if (removed_selection && !m_pageButtons.empty())
        SetSelection(static_cast<int>(std::min(n, m_pageButtons.size() - 1)));
    m_buttons_sizer->Remove(n);
    btn->Reparent(nullptr);
    btn->Destroy();
    m_sizer->Layout();
    Refresh(false);
}

bool TabButtonsListCtrl::SetPageImage(size_t n, const std::string &bmp_name)
{
    if (n >= m_pageButtons.size())
        return false;

    ScalableBitmap bitmap;
    if (!bmp_name.empty())
        bitmap = ScalableBitmap(this, bmp_name, 14);
    m_pageButtons[n]->SetBitmap(bitmap);

    return true;
}

void TabButtonsListCtrl::SetPageText(size_t n, const wxString &strText)
{
    TabButton *btn = m_pageButtons[n];
    btn->SetLabel(strText);
}

wxString TabButtonsListCtrl::GetPageText(size_t n) const
{
    TabButton *btn = m_pageButtons[n];
    return btn->GetLabel();
}

const wxSize& TabButtonsListCtrl::GetPaddingSize(size_t n) {
    return m_pageButtons[n]->GetPaddingSize();
}

void TabButtonsListCtrl::SetPaddingSize(const wxSize& size) {
    for (auto& btn : m_pageButtons) {
        btn->SetPaddingSize(size);
    }
}

//#endif // _WIN32


