#ifndef slic3r_ZHSplash_hpp_
#define slic3r_ZHSplash_hpp_

#include <wx/frame.h>
#include <wx/timer.h>
#include <wx/string.h>
#include <wx/gdicmn.h>
#include <wx/bitmap.h>
#include <wx/weakref.h>
#include <atomic>
#include <chrono>
#include <memory>

class wxGraphicsContext;
class wxMemoryDC;

namespace Slic3r { namespace GUI {

// Animated, double-buffered startup screen. Time-based (wall clock) so the motion
// stays smooth regardless of how long the heavy init work blocks the event loop.
class ZHSplashScreen : public wxFrame
{
public:
    explicit ZHSplashScreen(wxPoint pos = wxDefaultPosition);
    ~ZHSplashScreen() override;

    void SetText(const wxString& text);
    bool Destroy() override;

    // Keeps the event loop (and thus the animation) running for a short burst while
    // startup is otherwise blocking, so the intro actually plays instead of freezing.
    void PumpAnimation(int milliseconds);

    // Starts the closing transition. Called once the main window is up: the scene
    // collapses into the background and the window removes itself.
    void BeginOutro();

    // Closes the splash after the given delay, so the main window can paint first.
    void ScheduleOutro(int delay_ms);

    // Arms the handover from the target's first completed paint. The weak
    // reference keeps a late paint safe if the splash was closed elsewhere.
    void ScheduleOutroAfterPaint(wxWindow* target, int delay_ms);

    // Draws one animation frame; exposed so the scene can be rendered offscreen for tests.
    static void RenderScene(wxMemoryDC& dc, const wxSize& size, double t, const wxString& status, double dpi, double outro = 0.0);

private:
    void OnPaint(wxPaintEvent& event);
    void OnTimer(wxTimerEvent& event);
    double elapsed() const;
    double outro_progress() const;

    wxTimer m_timer;
    wxBitmap m_frame;
    wxString m_status;
    std::chrono::steady_clock::time_point m_start;
    bool m_motion_enabled{true};
    std::chrono::steady_clock::time_point m_outro_start;
    std::chrono::steady_clock::time_point m_outro_at;
    bool m_outro{false};
    bool m_outro_pending{false};
    bool m_handover_waiting{false};
    std::chrono::steady_clock::time_point m_handover_deadline;
    std::shared_ptr<std::atomic_bool> m_handover_painted;
};

}} // namespace Slic3r::GUI

#endif // slic3r_ZHSplash_hpp_
