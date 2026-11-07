#pragma once
#include <JuceHeader.h>

// Shared audio file helpers for the UI: format detection, length and cached waveform thumbnails.
class SampleLibrary
{
public:
    SampleLibrary();

    static bool isAudioFile (const juce::File&);
    static juce::String audioWildcard() { return "*.wav;*.mp3;*.flac;*.ogg;*.aif;*.aiff"; }

    double lengthSeconds (const juce::File&);          // 0 if unreadable
    juce::AudioThumbnail& thumbnail (const juce::File&); // created on first use, loads in the background

    // Draws a waveform (or a placeholder while it loads) into r.
    void drawWaveform (juce::Graphics&, const juce::File&, juce::Rectangle<float> r, juce::Colour colour,
                       double startSec = 0.0, double endSec = -1.0);

    juce::AudioFormatManager formats;

private:
    juce::AudioThumbnailCache cache { 256 };
    std::map<juce::String, std::unique_ptr<juce::AudioThumbnail>> thumbs;
    std::map<juce::String, double> lengths;
};
