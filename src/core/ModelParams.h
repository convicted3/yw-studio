#pragma once

// The knobs of the FL engines that keep their settings in the channel (ChannelDef::pluginState / modelParams), as
// automatable parameters (round 168n): the channel window's designed editor builds one knob per parameter (named
// EngineParam::name), its right-click menu creates automation clips on AutomationTarget { channelParam, channel, 0,
// index }, the Song reads / writes the value in the channel's state and the engine takes automation through
// Instrument::setParameter (index, 0…1) without restarting its voices.
//
// Each engine's table is listed with ONE line in core/ModelParamList.inc:
//     YW_MODEL_PARAMS (toxic, toxicModelTable)   // → yw::modelparams::Table yw::modelparams::toxicModelTable();
// defined in the engine's own file (or here for the plain "float P[i] in the state" engines).

#include <optional>
#include <string>
#include <vector>

#include "core/EngineParams.h"
#include "core/Song.h"

namespace yw::modelparams
{
struct Table
{
    const std::vector<EngineParam>* params = nullptr; // sorted by index
    // the raw value of a parameter in a channel's state (nullopt: the state can't be read)
    std::optional<float> (*get) (const ChannelDef&, const EngineParam&) = nullptr;
    // writes a raw value (already within the parameter's range); false when the state can't be written
    bool (*set) (ChannelDef&, const EngineParam&, float raw) = nullptr;
};

const Table* tableOf (InstrumentKind kind);
const EngineParam* find (InstrumentKind kind, int index);
const EngineParam* findByName (InstrumentKind kind, const std::string& name);

std::optional<float> rawValue (const ChannelDef& channel, int index);
bool setRawValue (ChannelDef& channel, int index, float raw);
// 0…1 over the parameter's range (automation clips, MIDI links)
std::optional<float> value01 (const ChannelDef& channel, int index);
bool setValue01 (ChannelDef& channel, int index, float value);

// Tables of the engines whose parameters are floats P[i] at an offset of their saved state
Table toxicModelTable();
Table poizoneModelTable();
} // namespace yw::modelparams
