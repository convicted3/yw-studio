#include "SampleLibrary.h"

SampleLibrary::SampleLibrary()
{
    formats.registerBasicFormats();
}

bool SampleLibrary::isAudioFile (const juce::File& f)
{
    const auto ext = f.getFileExtension().toLowerCase();
    return f.existsAsFile() && (ext == ".wav" || ext == ".mp3" || ext == ".flac" || ext == ".ogg"
                                || ext == ".aif" || ext == ".aiff");
}

double SampleLibrary::lengthSeconds (const juce::File& f)
{
    const auto key = f.getFullPathName();
    if (auto it = lengths.find (key); it != lengths.end()) return it->second;

    double len = 0.0;
    if (std::unique_ptr<juce::AudioFormatReader> r { formats.createReaderFor (f) })
        if (r->sampleRate > 0) len = (double) r->lengthInSamples / r->sampleRate;
    lengths[key] = len;
    return len;
}

juce::AudioThumbnail& SampleLibrary::thumbnail (const juce::File& f)
{
    auto& t = thumbs[f.getFullPathName()];
    if (t == nullptr)
    {
        t = std::make_unique<juce::AudioThumbnail> (256, formats, cache);
        t->setSource (new juce::FileInputSource (f));
    }
    return *t;
}

void SampleLibrary::drawWaveform (juce::Graphics& g, const juce::File& f, juce::Rectangle<float> r, juce::Colour colour,
                                  double startSec, double endSec)
{
    if (r.getWidth() < 2.0f || r.getHeight() < 2.0f) return;
    auto& t = thumbnail (f);
    const double total = t.getTotalLength() > 0.0 ? t.getTotalLength() : lengthSeconds (f);
    if (endSec < 0.0) endSec = total;

    if (t.getNumChannels() == 0 || total <= 0.0)
    {
        g.setColour (colour.withAlpha (0.35f));
        g.fillRect (r.withSizeKeepingCentre (r.getWidth(), 1.0f));
        return;
    }

    g.setColour (colour);
    if (t.getNumChannels() >= 2 && r.getHeight() > 40.0f)
        t.drawChannels (g, r.toNearestInt(), startSec, endSec, 1.0f);
    else
        t.drawChannel (g, r.toNearestInt(), startSec, endSec, 0, 1.0f);
}
