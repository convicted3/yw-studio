#pragma once

#include <functional>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/KeyMap.h"

namespace yw
{
// The Shortcut manager (FL Studio's "Manage shortcuts", interpreted): every action of YW's key map with where it
// works and its keys; search by name or key, change a shortcut by pressing the new keys (a clash with another
// action of the same panel is shown first), clear, reset one or all. Saved at once in %APPDATA%\YW\keys.json
// (only the changed actions); `onChanged` reloads the key map in the main window.
void showShortcutManager (std::function<void()> onChanged);

// The content, for --snapshot mode (`persistent` false: a sample change, nothing saved).
std::unique_ptr<juce::Component> makeShortcutManagerPanel (bool persistent, std::function<void()> onChanged = {});

// The key map file (%APPDATA%\YW\keys.json).
juce::File keyMapFile();

// JUCE key event -> YW key map chord.
keys::Chord chordFromKeyPress (const juce::KeyPress& key);
} // namespace yw
