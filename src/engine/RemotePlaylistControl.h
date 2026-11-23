#pragma once
#include <juce_core/juce_core.h>
#include <functional>
namespace yw
{
class AudioEngine;
bool isRemotePlaylistOperation (const juce::String& op);
juce::var dispatchRemotePlaylist (AudioEngine&, const juce::String& op, const juce::var& args, const std::function<bool()>& cancelled = {});
} // namespace yw
