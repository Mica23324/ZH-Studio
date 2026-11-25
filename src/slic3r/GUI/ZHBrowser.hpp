#ifndef slic3r_GUI_ZHBrowser_hpp_
#define slic3r_GUI_ZHBrowser_hpp_

#include "slic3r/GUI/ZHBrowser.hpp"
#include "libslic3r/NetworkPolicy.hpp"
#include <wx/utils.h>

namespace Slic3r {
namespace GUI {

// Single choke point for links that leave the application. Offline mode must
// never hand an HTTP(S) or custom-protocol URL to the operating system.
inline bool zh_open_external_browser(const wxString& url, int flags = 0)
{
    if (NetworkPolicy::is_offline())
        return false;
    return ::wxLaunchDefaultBrowser(url, flags);
}

} // namespace GUI
} // namespace Slic3r

#endif // slic3r_GUI_ZHBrowser_hpp_
