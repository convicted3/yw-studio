#pragma once
// Whether plugin and channel windows (and the floating panels) dock inside the main window as native child windows
// (issue #116: FL's docked plugin windows) — the platform rule, without JUCE so the core tests can check it.
// The docking glue (click raise, physical placement, stacking checks, key forwarding) is written for Win32 only, so:
// - Windows: docked by default; YW_DOCKED_PLUGINS=0 turns it off.
// - Linux / BSD (issue #165: X11 child windows lose text focus, leave stale pictures, have no frame) and macOS (no
//   docking glue either; rollup 0.1.28 after round 190gm): the system's own windows ("Detached", as before issue #116)
//   unless YW_DOCKED_PLUGINS=1.

#include <string_view>

namespace yw::dockpolicy
{
constexpr bool dockingOn (std::string_view ywDockedPlugins, bool windows) noexcept
{
    return windows ? ywDockedPlugins != "0" : ywDockedPlugins == "1";
}
} // namespace yw::dockpolicy
