#ifndef slic3r_NetworkPolicy_hpp_
#define slic3r_NetworkPolicy_hpp_

#include <atomic>

namespace Slic3r {
namespace NetworkPolicy {

// Process-wide, wx-independent network policy. Sources are deliberately
// monotonic: once a source enables offline mode, no later CLI/config value can
// turn networking back on for that process.
inline std::atomic<bool> cli_offline{false};
inline std::atomic<bool> config_offline{false};

inline bool is_offline() noexcept
{
    return cli_offline.load(std::memory_order_acquire) ||
           config_offline.load(std::memory_order_acquire);
}

inline void enable_cli_offline() noexcept
{
    cli_offline.store(true, std::memory_order_release);
}

inline void enable_config_offline() noexcept
{
    config_offline.store(true, std::memory_order_release);
}

} // namespace NetworkPolicy
} // namespace Slic3r

#endif // slic3r_NetworkPolicy_hpp_
