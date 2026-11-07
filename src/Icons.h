#pragma once
#include <JuceHeader.h>

// All YW Studio icons, drawn as vectors.
namespace icons
{
    enum class Id
    {
        none, arrowRight, arrowLeft, arrowDown, back, up, undo, play, stop, record,
        metronome, waitInput, countdown, blend, loopRecord, eye,
        reset, mic, help, save, saveNew, chat, download,
        minimise, maximise, close,
        typingKeyboard, stepEdit, slide, link, recFilter, magnet,
        playlist, pianoRoll, channelRack, mixer, browser, project, plug, catalog, globe,
        speaker, graph, steps, plus, star, folder, file, wave, automation, keys,
        pencil, brush, erase, mute, slip, slice, select, zoom, listen,
        tags, search, cloud, preset, sampler
    };

    void draw (juce::Graphics& g, Id id, juce::Rectangle<float> r, juce::Colour c);

    // The YW logo as a vector path in a 304 x 192 box.
    const juce::Path& logoPath();
    constexpr float logoWidth = 304.0f, logoHeight = 192.0f, logoSplitX = 111.0f; // Y | W split line
}
