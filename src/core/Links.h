#pragma once

// YW Studio's outside addresses that the app opens in the browser. One constant per address: change it here and
// every menu item, window and button that opens it follows.
namespace yw::links
{
// The donation page (round 190fq): Help › Support YW Studio..., the About window and the start-up note open it.
// Donating is optional and never unlocks anything. (Also in .github/FUNDING.yml, README.md and website/_src/build.py.)
inline constexpr const char* donate = "https://yolo.su";
} // namespace yw::links
