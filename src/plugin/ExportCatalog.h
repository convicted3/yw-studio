#pragma once

// YW's own engines that are built as VST3 / CLAP plugins for other hosts (round 180b, docs/PLUGIN_EXPORT.md): the entries
// of plugin/plugins.def with built = 1 (the CLAP) or 2 (the CLAP and a VST3 of its own). Only engines that need no FL Studio
// data files.

#include <array>

#include "core/YwBanks.h"
#include "core/Song.h"

namespace yw::plugexport
{
struct Entry
{
    const char* name;     // the plugin's name in the host
    bool instrument;
    EffectKind effect;    // when ! instrument
    InstrumentKind synth; // when instrument
    const char* clapId;
};

#define YW_PLUGIN_ENTRY_0(...)
#define YW_PLUGIN_ENTRY_1(name, instrument, effect, synth, clapId) Entry { name, instrument != 0, EffectKind::effect, InstrumentKind::synth, clapId },
#define YW_PLUGIN_ENTRY_2 YW_PLUGIN_ENTRY_1
#define YW_PLUGIN(name, instrument, effect, synth, clapId, code, category, built) YW_PLUGIN_ENTRY_##built (name, instrument, effect, synth, clapId)
inline constexpr std::array catalog {
#include "plugin/plugins.def"
};
#undef YW_PLUGIN
#undef YW_PLUGIN_ENTRY_0
#undef YW_PLUGIN_ENTRY_1
#undef YW_PLUGIN_ENTRY_2

// An instrument entry's channel in a new plugin: its engine's first bank preset, or (Tetra and Braid have no bank: rollup
// 0.1.31 step 6) the engine at its own defaults
inline ChannelDef instrumentChannel (const Entry& e)
{
    if (auto channel = banks::defaultChannel (e.synth, e.name))
        return *channel;
    ChannelDef channel;
    channel.instrument = e.synth;
    channel.name = e.name;
    return channel;
}
} // namespace yw::plugexport
