#include "ZHSplash.hpp"
#include "ZHTheme.hpp"
#include "ZHRounded.hpp"
#include "ZHMotion.hpp"

#include <libslic3r/libslic3r.h>
#include <libslic3r/Log.hpp>

#include <wx/dcbuffer.h>
#include <wx/display.h>
#include <wx/graphics.h>
#include <wx/settings.h>
#include <wx/app.h>
#include <wx/utils.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <memory>

namespace Slic3r { namespace GUI {
namespace {

double clamp01(double v) { return v < 0.0 ? 0.0 : (v > 1.0 ? 1.0 : v); }
double ease(double v) { v = clamp01(v); return v * v * (3.0 - 2.0 * v); }

wxColour with_alpha(const wxColour& c, double a)
{
    return wxColour(c.Red(), c.Green(), c.Blue(), static_cast<unsigned char>(clamp01(a) * 255.0));
}

struct Palette
{
    wxColour bg_top{6, 11, 20};
    wxColour bg_bot{12, 21, 36};
    wxColour accent = ZHTheme::accent();
    wxColour secondary = ZHTheme::hovered();
    wxColour light{241, 245, 249};
    wxColour muted{143, 162, 187};
    wxColour track{23, 35, 58};
    wxColour on_accent{4, 33, 47};
};

wxFont make_font(double pt, bool bold)
{
    wxFontInfo info(pt);
    info.Family(wxFONTFAMILY_SWISS);
    info.Weight(bold ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL);
    return wxFont(info);
}

const wxFont& small_bold_font()
{
    static const wxFont font = make_font(10, true);
    return font;
}

const wxFont& title_bold_font()
{
    static const wxFont font = make_font(30, true);
    return font;
}

const wxFont& slogan_bold_font()
{
    static const wxFont font = make_font(15, true);
    return font;
}

const wxFont& small_regular_font()
{
    static const wxFont font = make_font(10, false);
    return font;
}

void rounded_rect_gc(wxGraphicsContext* gc, double x, double y, double w, double h, double r)
{
    wxGraphicsPath path = gc->CreatePath();
    path.AddRoundedRectangle(x, y, w, h, r);
    gc->FillPath(path);
}

void radial_glow(wxGraphicsContext* gc, double cx, double cy, double radius, wxColour color, double alpha)
{
    wxGraphicsBrush brush = gc->CreateRadialGradientBrush(cx, cy, cx, cy, radius,
        with_alpha(color, alpha), with_alpha(color, 0.0));
    gc->SetBrush(brush);
    gc->DrawEllipse(cx - radius, cy - radius, radius * 2, radius * 2);
}

// Inverse of radial_glow: transparent in the centre, opaque towards the rim. Used to
// melt the scene into the background while the splash closes.
void radial_fade(wxGraphicsContext* gc, double cx, double cy, double radius, wxColour color, double alpha)
{
    if (alpha <= 0.0) return;
    wxGraphicsBrush brush = gc->CreateRadialGradientBrush(cx, cy, cx, cy, radius,
        with_alpha(color, 0.0), with_alpha(color, alpha));
    gc->SetBrush(brush);
    gc->DrawEllipse(cx - radius, cy - radius, radius * 2, radius * 2);
}

// The ZH brand mark, drawn as vectors so it always matches the app palette.
void draw_logo(wxGraphicsContext* gc, const Palette& p, double ox, double oy, double size, double t)
{
    const double s = size / 256.0;
    auto X = [&](double v) { return ox + v * s; };
    auto Y = [&](double v) { return oy + v * s; };

    wxGraphicsPath tile = gc->CreatePath();
    tile.AddRoundedRectangle(X(4), Y(4), 248 * s, 248 * s, 54 * s);
    gc->SetBrush(wxBrush(with_alpha(p.bg_bot, 0.85)));
    gc->FillPath(tile);

    static const double z[][2] = {{38,64},{122,64},{122,83},{66,164},{122,164},{122,187},{34,187},{34,168},{91,87},{38,87}};
    static const double h[][2] = {{142,64},{165,64},{165,111},{197,111},{197,64},{220,64},{220,187},{197,187},{197,135},{165,135},{165,187},{142,187}};
    gc->SetBrush(wxBrush(p.accent));
    for (auto* poly : {z, h}) {
        wxGraphicsPath path = gc->CreatePath();
        path.MoveToPoint(X(poly[0][0]), Y(poly[0][1]));
        for (int i = 1; i < (poly == z ? 10 : 12); ++i) path.AddLineToPoint(X(poly[i][0]), Y(poly[i][1]));
        path.CloseSubpath();
        gc->FillPath(path);
    }
    const double underline = ease((t - 0.12) / 0.25);
    if (underline > 0) {
        gc->SetPen(wxPen(p.secondary, std::max(1.0, 4 * s)));
        gc->StrokeLine(X(38), Y(210), X(38 + 182 * underline), Y(210));
    }
}

// Deliberately restrained: one gradient, one static glow, one moving progress shimmer.
// Fewer moving parts mean a calmer startup and no flicker on slower machines.
void draw_shapes(wxGraphicsContext* gc, double w, double h, double t, double dpi, const Palette& p)
{
    auto D = [&](double v) { return v * dpi; };
    wxGraphicsBrush bg = gc->CreateLinearGradientBrush(0, 0, 0, h, p.bg_top, p.bg_bot);
    gc->SetBrush(bg);
    gc->DrawRectangle(0, 0, w, h);

    const double L = D(132);
    const double cx = w / 2, cy = h * 0.28;
    radial_glow(gc, cx, cy, L * 0.95, p.accent, 0.18);

    gc->SetBrush(wxBrush(p.accent));
    gc->DrawRectangle(0, 0, w, 2);

    gc->SetPen(wxPen(with_alpha(p.secondary, 0.16), 1));
    gc->SetBrush(*wxTRANSPARENT_BRUSH);
    gc->DrawRectangle(0.5, 0.5, w - 1, h - 1);

    draw_logo(gc, p, cx - L / 2, cy - L / 2, L, t);

    // Progress track with a single smooth shimmer.
    double tx = D(48), tw = w - D(96), ty = h - D(70), th = D(4);
    gc->SetBrush(wxBrush(p.track));
    rounded_rect_gc(gc, tx, ty, tw, th, th / 2);
    const double pp = std::fmod(t * 0.45, 1.45) - 0.22;
    const double sx = tx + pp * tw, sw = tw * 0.22;
    gc->Clip(tx, ty, tw, th);
    wxGraphicsBrush shim = gc->CreateLinearGradientBrush(sx, 0, sx + sw, 0,
        with_alpha(p.accent, 0.0), with_alpha(p.secondary, 0.85));
    gc->SetBrush(shim);
    gc->DrawRectangle(sx, ty, sw, th);
    gc->ResetClip();
}

// All text layers, drawn with the graphics context into the offscreen frame.
void draw_text(wxGraphicsContext* gc, double w, double h, double t, const wxString& status, double dpi, const Palette& p)
{
    auto D = [&](double v) { return v * dpi; };
    const wxFont& small_bold = small_bold_font();
    const wxFont& title_bold = title_bold_font();
    const wxFont& slogan_bold = slogan_bold_font();
    const wxFont& small_regular = small_regular_font();
    const wxRealPoint font_dpi(96.0 * dpi, 96.0 * dpi);
    auto setfont = [&](const wxFont& value, const wxColour& c) {
        // A windowless wxMemoryDC uses the system DPI on wxMSW. Explicit DPI
        // keeps text aligned with the DPI-scaled geometry on mixed-DPI displays.
        gc->SetFont(gc->GetRenderer()->CreateFontAtDPI(value, font_dpi, c));
    };
    auto drawspaced = [&](const wxString& s, double x, double y, double sp, double frac = 1.0) {
        const size_t n = static_cast<size_t>(clamp01(frac) * s.size() + 0.999);
        if (sp <= 0.0) {
            gc->DrawText(s.Left(n), x, y);
            return;
        }
        for (size_t i = 0; i < n && i < s.size(); ++i) {
            wxString ch = s.Mid(i, 1);
            gc->DrawText(ch, x, y);
            double ew = 0, eh = 0;
            gc->GetTextExtent(ch, &ew, &eh);
            x += ew + sp;
        }
    };
    auto spacedwidth = [&](const wxString& s, double sp) {
        double total = 0;
        for (size_t i = 0; i < s.size(); ++i) {
            double cw = 0, ch = 0;
            gc->GetTextExtent(s.Mid(i, 1), &cw, &ch);
            total += cw + (i + 1 < s.size() ? sp : 0);
        }
        return total;
    };

    setfont(small_bold, with_alpha(p.secondary, ease(t / 0.18)));
    wxString eyebrow = wxString::FromUTF8("ZH-STUDIO / ALPHA");
    double ebw = spacedwidth(eyebrow, 3);
    drawspaced(eyebrow, (w - ebw) / 2, D(26), 3);

    const double L = D(132);
    const double cy = h * 0.28;
    setfont(title_bold, p.light);
    wxString word = wxString::FromUTF8("ZH-Studio");
    double ww = 0, wh = 0;
    gc->GetTextExtent(word, &ww, &wh);
    setfont(small_bold, p.on_accent);
    wxString chip = wxString::FromUTF8("ALPHA");
    double cw = spacedwidth(chip, 2) + D(16);
    double chh = D(20);
    double total = ww + D(12) + cw;
    double wx0 = (w - total) / 2;
    double wy = cy + L / 2 + D(30);
    double reveal = ease((t - 0.14) / 0.24);
    setfont(title_bold, p.light);
    drawspaced(word, wx0, wy, 0, reveal);
    if (reveal > 0.99) {
        gc->SetBrush(wxBrush(p.accent));
        rounded_rect_gc(gc, wx0 + ww + D(12), wy + (wh - chh) / 2, cw, chh, D(5));
        setfont(small_bold, p.on_accent);
        drawspaced(chip, wx0 + ww + D(20), wy + (wh - chh) / 2 + D(4), 2);
    }

    wxString line1 = wxString::FromUTF8("THIS IS NOT NOW.");
    wxString line2 = wxString::FromUTF8("IT'S THE FUTURE.");
    setfont(slogan_bold, p.light);
    double l1w = 0, l1h = 0; gc->GetTextExtent(line1, &l1w, &l1h);
    double y1 = wy + wh + D(26);
    drawspaced(line1, (w - l1w) / 2, y1, 0, ease((t - 0.20) / 0.20));
    setfont(slogan_bold, p.secondary);
    double l2w = 0, l2h = 0; gc->GetTextExtent(line2, &l2w, &l2h);
    double y2 = y1 + l1h + D(8);
    drawspaced(line2, (w - l2w) / 2, y2, 0, ease((t - 0.30) / 0.20));

    setfont(small_regular, with_alpha(p.muted, ease((t - 0.36) / 0.14)));
    wxString ver = wxString::FromUTF8("V ") + wxString::FromUTF8(SLIC3R_VERSION) + wxString::FromUTF8("  ·  ") + wxString::FromUTF8(SLIC3R_APP_EDITION);
    double vw = 0, vh = 0;
    gc->GetTextExtent(ver, &vw, &vh);
    gc->DrawText(ver, (w - vw) / 2, y2 + l2h + D(18));

    if (!status.empty()) {
        setfont(small_regular, p.muted);
        double sw2 = 0, sh2 = 0; gc->GetTextExtent(status, &sw2, &sh2);
        // Centre the label together with its pulsing dots, otherwise the line sits off-axis.
        const double dot = D(3), gap = D(7);
        const double dots_w = gap * 2 + dot;
        const double sx = (w - (sw2 + D(9) + dots_w)) / 2;
        gc->DrawText(status, sx, h - D(70) + D(13));
        // Three pulsing loading dots trailing the status line.
        for (int i = 0; i < 3; ++i) {
            double pulse = 0.35 + 0.65 * clamp01(std::sin(t * 4.0 - i * 0.7));
            gc->SetBrush(wxBrush(with_alpha(p.secondary, pulse)));
            gc->DrawEllipse(sx + sw2 + D(9) + i * gap, h - D(70) + D(17), dot, dot);
        }
    }
}

} // namespace

void ZHSplashScreen::RenderScene(wxMemoryDC& dc, const wxSize& size, double t, const wxString& status, double dpi, double outro)
{
    const double w = size.x;
    const double h = size.y;
    Palette p;
    std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
    if (!gc) return;

    const double close = clamp01(outro);
    if (close <= 0.0) {
        draw_shapes(gc.get(), w, h, t, dpi, p);
        draw_text(gc.get(), w, h, t, status, dpi, p);
        return;
    }

    // Closing transition: the composition settles into the background colour while the
    // splash hands over to the main window.
    gc->SetBrush(wxBrush(p.bg_bot));
    gc->DrawRectangle(0, 0, w, h);
    const double scale = 1.0 - 0.08 * close;
    gc->PushState();
    gc->Translate(w / 2, h / 2);
    gc->Scale(scale, scale);
    gc->Translate(-w / 2, -h / 2);
    draw_shapes(gc.get(), w, h, t, dpi, p);
    draw_text(gc.get(), w, h, t, status, dpi, p);
    gc->PopState();
    radial_fade(gc.get(), w / 2, h / 2, std::max(w, h) * 0.7, p.bg_bot, close);
}

ZHSplashScreen::ZHSplashScreen(wxPoint pos)
    : wxFrame(nullptr, wxID_ANY, wxString::FromUTF8("ZH-Studio Alpha"), pos, wxDefaultSize,
        wxBORDER_NONE | wxFRAME_NO_TASKBAR | wxSTAY_ON_TOP)
    , m_start(std::chrono::steady_clock::now())
    , m_motion_enabled(ZHMotion::enabled())
{
    if (!m_motion_enabled)
        m_start -= std::chrono::seconds(1); // Render the settled composition immediately.
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    const wxSize splash_size = FromDIP(wxSize(520, 600));
    SetSize(splash_size);
    // The splash is always centred. The position handed over by the caller only tells
    // which display to use - taking it literally would stick the window to the screen
    // corner the main window was in.
    unsigned int display_index = 0;
    if (pos != wxDefaultPosition)
        display_index = wxDisplay::GetFromPoint(pos);
    const wxRect area = wxDisplay(display_index).GetClientArea();
    SetPosition(wxPoint(area.x + (area.width - splash_size.GetWidth()) / 2,
                        area.y + (area.height - splash_size.GetHeight()) / 2));
    SetBackgroundColour(wxColour(12, 21, 36));

    ZHRounded::Apply(this, 2, RGB(45, 65, 92));

    Bind(wxEVT_PAINT, &ZHSplashScreen::OnPaint, this);
    m_timer.SetOwner(this);
    Bind(wxEVT_TIMER, &ZHSplashScreen::OnTimer, this);
    if (m_motion_enabled && !m_timer.Start(33)) {
        BOOST_LOG_TRIVIAL(warning) << "Could not start splash animation timer; using a static splash";
        m_motion_enabled = false;
        m_start -= std::chrono::seconds(1);
    }
    Show();
    Raise();
    Update();
}

ZHSplashScreen::~ZHSplashScreen() { m_timer.Stop(); }

bool ZHSplashScreen::Destroy()
{
    m_timer.Stop();
    return wxFrame::Destroy();
}

void ZHSplashScreen::SetText(const wxString& text)
{
    m_status = text;
    Refresh();
    Update();
}

void ZHSplashScreen::PumpAnimation(int milliseconds)
{
    if (!m_motion_enabled) {
        Refresh();
        Update();
        return;
    }

    // The splash plays while OnInit is still running, i.e. before the wx event
    // loop exists. Neither the timer nor wxYield() delivers events there, so the
    // frames are rendered synchronously: Refresh() marks the window dirty and
    // Update() paints it right away, which does work outside of the main loop.
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(milliseconds);
    constexpr auto frame_interval = std::chrono::milliseconds(33);
    auto next_frame = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() < deadline) {
        Refresh();
        Update();
        next_frame += frame_interval;
        const auto now = std::chrono::steady_clock::now();
        if (next_frame > now)
            wxMilliSleep(static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(next_frame - now).count()));
    }
}

void ZHSplashScreen::BeginOutro()
{
    if (!m_motion_enabled) {
        Destroy();
        return;
    }
    // Runs inside the event loop, so the timer keeps the scene alive while it closes.
    if (m_outro) return;
    m_outro = true;
    m_outro_start = std::chrono::steady_clock::now();
    Refresh();
}

void ZHSplashScreen::ScheduleOutro(int delay_ms)
{
    if (!m_motion_enabled) {
        Destroy();
        return;
    }
    // The regular repaint timer picks the deadline up, so no second timer is needed.
    m_outro_pending = true;
    m_outro_at = std::chrono::steady_clock::now() + std::chrono::milliseconds(delay_ms);
}

void ZHSplashScreen::ScheduleOutroAfterPaint(wxWindow* target, int delay_ms)
{
    if (!target) {
        ScheduleOutro(0);
        return;
    }

    auto painted = std::make_shared<std::atomic_bool>(false);
    m_handover_painted = painted;
    m_handover_waiting = true;
    // A hidden or permanently occluded target must not leave the splash stranded.
    // The regular animation timer provides a bounded fallback without adding a
    // second timer or weakening the normal first-paint synchronization.
    m_handover_deadline = std::chrono::steady_clock::now() +
        std::chrono::milliseconds(std::max(delay_ms, 0) + 1000);
    wxWeakRef<ZHSplashScreen> weak_splash(this);
    target->Bind(wxEVT_PAINT, [weak_splash, painted, delay_ms](wxPaintEvent& event) {
        // Preserve the target's own paint handler, then run after the complete
        // paint event dispatch rather than merely after Show().
        event.Skip();
        if (painted->exchange(true))
            return;
        wxTheApp->CallAfter([weak_splash, delay_ms] {
            if (weak_splash)
                weak_splash->ScheduleOutro(delay_ms);
        });
    });
}

double ZHSplashScreen::outro_progress() const
{
    if (!m_outro) return 0.0;
    const double e = std::chrono::duration<double>(std::chrono::steady_clock::now() - m_outro_start).count();
    return clamp01(e / 0.32);
}

double ZHSplashScreen::elapsed() const
{
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - m_start).count();
}

void ZHSplashScreen::OnTimer(wxTimerEvent&)
{
    if (!m_motion_enabled) {
        Destroy();
        return;
    }
    const auto now = std::chrono::steady_clock::now();
    if (m_handover_waiting && m_handover_painted) {
        if (m_handover_painted->load(std::memory_order_acquire)) {
            m_handover_waiting = false;
        } else if (now >= m_handover_deadline) {
            m_handover_waiting = false;
            ScheduleOutro(0);
        }
    }
    if (m_outro_pending && now >= m_outro_at) {
        m_outro_pending = false;
        BeginOutro();
    }
    if (m_outro && outro_progress() >= 1.0) {
        m_timer.Stop();
        Destroy();
        return;
    }
    Refresh();
}

void ZHSplashScreen::OnPaint(wxPaintEvent&)
{
    // Render offscreen (proven reliable for both vector and text layers) and blit the
    // finished frame; drawing text straight into the host window DC proved unreliable.
    const wxSize sz = GetClientSize();
    if (sz.x <= 0 || sz.y <= 0) return;
    if (!m_frame.IsOk() || m_frame.GetSize() != sz)
        m_frame = wxBitmap(sz.x, sz.y, 32);
    {
        wxMemoryDC mdc(m_frame);
        RenderScene(mdc, sz, elapsed(), m_status, GetDPIScaleFactor(), outro_progress());
    }
    wxPaintDC dc(this);
    dc.DrawBitmap(m_frame, 0, 0, false);
}

}} // namespace Slic3r::GUI
