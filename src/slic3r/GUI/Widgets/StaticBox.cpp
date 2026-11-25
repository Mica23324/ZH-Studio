#include "StaticBox.hpp"
#include "../GUI.hpp"
#include "../ZHMotion.hpp"
#include <wx/dcclient.h>
#include <wx/dcgraph.h>
#include <chrono>

namespace {

long long motion_now_ms()
{
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

constexpr int kMotionFrameMs = 16;
constexpr int kMotionDurationMs = 150;

} // namespace

BEGIN_EVENT_TABLE(StaticBox, wxWindow)

// catch paint events
//EVT_ERASE_BACKGROUND(StaticBox::eraseEvent)
EVT_PAINT(StaticBox::paintEvent)

END_EVENT_TABLE()

/*
 * Called by the system of by wxWidgets when the panel needs
 * to be redrawn. You can also trigger this call by
 * calling Refresh()/Update().
 */

StaticBox::StaticBox()
    : state_handler(this)
    , radius(8)
{
    border_color = StateColor(
        std::make_pair(0xF0F0F1, (int) StateColor::Disabled),
        std::make_pair(0xCECECE, (int) StateColor::Normal));
    m_motion_timer.SetOwner(this);
    Bind(wxEVT_TIMER, &StaticBox::on_motion_timer, this, m_motion_timer.GetId());
}

StaticBox::StaticBox(wxWindow* parent,
                   wxWindowID      id,
                   const wxPoint & pos,
                   const wxSize &  size, long style)
    : StaticBox()
{
    Create(parent, id, pos, size, style);
}

bool StaticBox::Create(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style)
{
    if (style & wxBORDER_NONE)
        border_width = 0;
    wxWindow::Create(parent, id, pos, size, style | wxFULL_REPAINT_ON_RESIZE);
    state_handler.attach({&border_color, &background_color, &background_color2});
    state_handler.update_binds();
    SetBackgroundColour(GetParentBackgroundColor(parent));
    return true;
}

void StaticBox::SetCornerRadius(double radius)
{
    this->radius = radius;
    Refresh();
}

void StaticBox::SetBorderStyle(wxPenStyle style)
{
    border_style = style;
    Refresh();
}

void StaticBox::SetBorderWidth(int width)
{
    border_width = width;
    Refresh();
}

void StaticBox::SetBorderColor(StateColor const &color)
{
    if (border_color != color) {
        border_color = color;
        state_handler.update_binds();
        Refresh();
    }
}

void StaticBox::SetBorderColorNormal(wxColor const &color)
{
    border_color.setColorForStates(color, 0);
    Refresh();
}

void StaticBox::SetBackgroundColor(StateColor const &color)
{
    if (background_color == color) return;
    background_color = color;
    state_handler.update_binds();
    Refresh();
}

void StaticBox::SetBackgroundColorNormal(wxColor const &color)
{
    background_color.setColorForStates(color, 0);
    Refresh();
}

void StaticBox::SetBackgroundColor2(StateColor const &color)
{
    if (background_color2 == color) return;
    background_color2 = color;
    state_handler.update_binds();
    Refresh();
}

wxColor StaticBox::GetParentBackgroundColor(wxWindow* parent)
{
    if (auto box = dynamic_cast<StaticBox*>(parent)) {
        if (box->background_color.count() > 0) {
            if (box->background_color2.count() == 0)
                return box->background_color.defaultColor();
            auto s = box->background_color.defaultColor();
            auto e = box->background_color2.defaultColor();
            int r = (s.Red() + e.Red()) / 2;
            int g = (s.Green() + e.Green()) / 2;
            int b = (s.Blue() + e.Blue()) / 2;
            return wxColor(r, g, b);
        }
    }
    if (parent)
        return parent->GetBackgroundColour();
    return *wxWHITE;
}

void StaticBox::ShowBadge(bool show)
{
    if (show && badge.name() != badge_icon_name) {
        badge = ScalableBitmap(this, badge_icon_name, 18);
        Refresh();
    } else if (!show && !badge.name().empty()) {
        badge = ScalableBitmap {};
        Refresh();
    }
}

void StaticBox::eraseEvent(wxEraseEvent& evt)
{
    // for transparent background, but not work
#ifdef __WXMSW__
    wxDC *dc = evt.GetDC();
    wxSize size = GetSize();
    wxClientDC dc2(GetParent());
    dc->Blit({0, 0}, size, &dc2, GetPosition());
#endif
}

void StaticBox::paintEvent(wxPaintEvent& evt)
{
    // depending on your system you may need to look at double-buffered dcs
    wxPaintDC dc(this);
    render(dc);
}

/*
 * Here we do the actual rendering. I put it in a separate
 * method so that it can work no matter what type of DC
 * (e.g. wxPaintDC or wxClientDC) is used.
 */
void StaticBox::render(wxDC& dc)
{
#ifdef __WXMSW__
    if (radius == 0) {
        doRender(dc);
        return;
    }

	wxSize size = GetSize();
    if (size.x <= 0 || size.y <= 0)
        return;
    wxMemoryDC memdc(&dc);
    if (!memdc.IsOk()) {
        doRender(dc);
        return;
    }
    wxBitmap bmp(size.x, size.y);
    memdc.SelectObject(bmp);
    //memdc.Blit({0, 0}, size, &dc, {0, 0});
    memdc.SetBackground(wxBrush(GetBackgroundColour()));
    memdc.Clear();
    {
        wxGCDC dc2(memdc);
        doRender(dc2);
    }

    memdc.SelectObject(wxNullBitmap);
	dc.DrawBitmap(bmp, 0, 0);
#else
    doRender(dc);
#endif
}

void StaticBox::begin_motion(const wxColour &bg, const wxColour &bg2, bool gradient, const wxColour &border)
{
    m_motion_from_bg     = m_motion_bg;
    m_motion_from_bg2    = m_motion_bg2;
    m_motion_from_border = m_motion_border;
    m_motion_to_bg       = bg;
    m_motion_to_bg2      = bg2;
    m_motion_to_border   = border;
    m_motion_target_gradient = gradient;
    // Keep rendering a gradient while fading from or towards one, so the
    // control never jumps between the two drawing paths mid-transition.
    m_motion_has_gradient = m_motion_has_gradient || gradient;
    m_motion_started_ms   = motion_now_ms();
    m_motion_animating    = true;
    if (!m_motion_timer.Start(kMotionFrameMs)) {
        m_motion_animating = false;
        m_motion_bg = bg;
        m_motion_bg2 = bg2;
        m_motion_border = border;
        m_motion_has_gradient = gradient;
    }
}

wxColour StaticBox::motion_tick()
{
    const float progress = std::min(1.f, static_cast<float>(motion_now_ms() - m_motion_started_ms) /
                                              static_cast<float>(kMotionDurationMs));
    const float eased = ZHMotion::ease_out(progress);
    m_motion_bg     = ZHMotion::blend(m_motion_from_bg, m_motion_to_bg, eased);
    m_motion_bg2    = ZHMotion::blend(m_motion_from_bg2, m_motion_to_bg2, eased);
    m_motion_border = ZHMotion::blend(m_motion_from_border, m_motion_to_border, eased);
    if (progress >= 1.f) {
        m_motion_animating = false;
        m_motion_timer.Stop();
        m_motion_bg     = m_motion_to_bg;
        m_motion_bg2    = m_motion_to_bg2;
        m_motion_border = m_motion_to_border;
        m_motion_has_gradient = m_motion_target_gradient;
    }
    return m_motion_bg;
}

void StaticBox::on_motion_timer(wxTimerEvent &)
{
    if (!m_motion_animating)
        return;
    motion_tick();
    Refresh(false);
}

void StaticBox::doRender(wxDC& dc)
{
    wxSize size = GetSize();
    int states = state_handler.states();

    const wxColour target_bg = background_color.count() > 0 ? background_color.colorForStates(states)
                                                            : GetBackgroundColour();
    const bool     target_gradient = background_color2.count() > 0;
    const wxColour target_bg2 = target_gradient ? background_color2.colorForStates(states) : target_bg;
    const wxColour target_border = border_color.count() > 0 ? border_color.colorForStates(states) : target_bg;

    if (m_motion_last_states < 0) {
        m_motion_bg      = m_motion_to_bg = target_bg;
        m_motion_bg2     = m_motion_to_bg2 = target_bg2;
        m_motion_border  = m_motion_to_border = target_border;
        m_motion_has_gradient = target_gradient;
        m_motion_target_gradient = target_gradient;
        m_motion_last_states = states;
    } else if (m_motion_last_states != states || m_motion_to_bg != target_bg ||
               m_motion_to_bg2 != target_bg2 || m_motion_to_border != target_border ||
               m_motion_target_gradient != target_gradient) {
        m_motion_last_states = states;
        if (ZHMotion::enabled()) {
            begin_motion(target_bg, target_bg2, target_gradient, target_border);
        } else {
            m_motion_animating = false;
            m_motion_timer.Stop();
            m_motion_bg      = m_motion_to_bg = target_bg;
            m_motion_bg2     = m_motion_to_bg2 = target_bg2;
            m_motion_border  = m_motion_to_border = target_border;
            m_motion_has_gradient = target_gradient;
            m_motion_target_gradient = target_gradient;
        }
    }

    if (!m_motion_has_gradient) {
        if ((border_width && border_color.count() > 0) || background_color.count() > 0) {
            wxRect rc(0, 0, size.x, size.y);
            if (border_width && border_color.count() > 0) {
                if (dc.GetContentScaleFactor() == 1.0) {
                    int d  = floor(border_width / 2.0);
                    int d2 = floor(border_width - 1);
                    rc.x += d;
                    rc.width -= d2;
                    rc.y += d;
                    rc.height -= d2;
                } else {
                    int d  = 1;
                    rc.x += d;
                    rc.width -= d;
                    rc.y += d;
                    rc.height -= d;
                }
                dc.SetPen(wxPen(m_motion_border, border_width, border_style));
            } else {
                dc.SetPen(wxPen(m_motion_bg));
            }
            if (background_color.count() > 0)
                dc.SetBrush(wxBrush(m_motion_bg));
            else
                dc.SetBrush(wxBrush(GetBackgroundColour()));
            if (radius == 0) {
                dc.DrawRectangle(rc);
            }
            else {
                dc.DrawRoundedRectangle(rc, radius - border_width);
            }
        }
    }
    else {
        wxColor start = m_motion_bg;
        wxColor stop  = m_motion_bg2;
        int r = start.Red(), g = start.Green(), b = start.Blue();
        int dr = (int) stop.Red() - r, dg = (int) stop.Green() - g, db = (int) stop.Blue() - b;
        int lr = 0, lg = 0, lb = 0;
        for (int y = 0; y < size.y; ++y) {
            dc.SetPen(wxPen(wxColor(r, g, b)));
            dc.DrawLine(0, y, size.x, y);
            lr += dr; while (lr >= size.y) { ++r, lr -= size.y; } while (lr <= -size.y) { --r, lr += size.y; }
            lg += dg; while (lg >= size.y) { ++g, lg -= size.y; } while (lg <= -size.y) { --g, lg += size.y; }
            lb += db; while (lb >= size.y) { ++b, lb -= size.y; } while (lb <= -size.y) { --b, lb += size.y; }
        }
    }

    if (badge.bmp().IsOk()) {
        auto s = badge.bmp().GetScaledSize();
        dc.DrawBitmap(badge.bmp(), size.x - s.x, 0);
    }
}
