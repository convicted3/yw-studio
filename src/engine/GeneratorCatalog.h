#pragma once

#include "core/Song.h"
#include <juce_core/juce_core.h>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace yw { class AudioEngine; }
namespace yw::generators
{
// The shared generator list and factories for menus and control clients. Labels are
// English data here; UI callers translate them. No UI components or plugin scanning.
struct Entry
{
    std::string key;
    juce::String label, detail;
    int folder = 0; // 0 YW, 1 FL engines, 2 VFX, 3 scanned hosted plugins
    std::function<std::optional<ChannelDef>()> make;
    int section = 1;
    InstrumentKind kind = InstrumentKind::synth;
    bool available = true;
    juce::String unavailableReason, format, manufacturer;
};
std::vector<Entry> entries (AudioEngine&, bool withUnavailable = false);
}
