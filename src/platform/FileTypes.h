#pragma once

namespace juce
{
class File;
}

namespace yw
{
// Round 190gk: FL's "Get properties" (the Sampler waveform menu; GetSamplePropMenuClick 0x11f22a0 resolves the
// sample's file and calls ShellExecuteEx with the verb "properties", REA): the system's file properties window.
// Windows only; `fileProperties` is false elsewhere (the menu leaves the item out there).
constexpr bool fileProperties =
#if defined(_WIN32)
    true;
#else
    false;
#endif
bool showFileProperties (const juce::File& file);

// Registers .yw project files with Windows for the current user (no admin rights): double-click opens them in this
// YW.exe and Explorer shows YW's project icon. Rewrites the entries only when they point somewhere else, so a
// moved or updated exe takes the association over. Does nothing on other platforms.
// Linux AppImage: when YW's menu entry (made by setAppImageMenuEntry) points at another AppImage file — moved, or
// replaced by a newer version — it is rewritten for this one.
void registerFileTypes();

// Linux (round 161): YW runs from an AppImage ($APPIMAGE). AppImages are single files that integrate nothing by
// themselves; on request YW adds its applications-menu entry, its icon and the .yw file type to the user's
// ~/.local/share (no root), pointing at this AppImage, and removes them again.
bool runningAsAppImage();
bool appImageMenuEntryInstalled();
bool setAppImageMenuEntry (bool on);
} // namespace yw
