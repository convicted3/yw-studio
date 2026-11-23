#pragma once

// Offline mode (round 190nx, issue #233 follow-up): `YW_OFFLINE=1` in the environment, or `--offline` on the command line,
// guarantees that YW Studio opens no network connection of its own: no update check or update download, no crash report
// upload, no model download (the stem model), no Online Sessions (live session relay). The places that would connect ask
// `yw::offline::enabled()` first and say so instead (a message, never a silent failure). Files, Send project / Merge a
// part (a .ywpack file you send yourself), the Remote Control API (127.0.0.1) and the plugin sandbox are not network
// access and keep working. No JUCE here: the engine, the core and the tests share it.

#include <atomic>
#include <cstdlib>
#include <cstring>

namespace yw::offline
{
namespace detail
{
inline std::atomic<bool>& flag() noexcept
{
    static std::atomic<bool> set { false };
    return set;
}

// "1", "true", "yes", "on" or any other non-empty value except "0", "false", "no", "off"
inline bool envOn() noexcept
{
    const char* e = std::getenv ("YW_OFFLINE");
    if (e == nullptr || e[0] == '\0')
        return false;
    return std::strcmp (e, "0") != 0 && std::strcmp (e, "false") != 0 && std::strcmp (e, "no") != 0 && std::strcmp (e, "off") != 0;
}
} // namespace detail

// `--offline` (set once, early in main); tests also use it to turn the mode on and off
inline void set (bool on) noexcept { detail::flag() = on; }

inline bool enabled() noexcept { return detail::flag().load() || detail::envOn(); }

// English text of the reasons (callers pass them through tr())
inline constexpr const char* whyShort = "offline mode (YW_OFFLINE or --offline)";
} // namespace yw::offline
