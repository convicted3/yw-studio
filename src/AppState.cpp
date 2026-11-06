#include "AppState.h"

AppState::AppState()
{
    juce::PropertiesFile::Options o;
    o.applicationName     = "YW Studio";
    o.folderName          = "YW Studio";
    o.filenameSuffix      = ".settings";
    o.osxLibrarySubFolder = "Application Support";
    props.setStorageParameters (o);
}

AppState::~AppState()
{
    knownPlugins.removeChangeListener (this);
    deviceManager.removeAudioCallback (this);
    saveSettings();
    deviceManager.closeAudioDevice();
}

juce::File AppState::getAppDataFolder() const
{
    auto f = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("YW Studio");
    f.createDirectory();
    return f;
}

void AppState::loadSettings()
{
    auto& s = settings();
    transport.tempo        = s.getDoubleValue ("tempo", 130.0);
    transport.masterVolume = (float) s.getDoubleValue ("masterVolume", 0.8);
}

void AppState::initialiseAudio()
{
    auto saved = settings().getXmlValue ("audioDeviceState");
    auto err = deviceManager.initialise (2, 2, saved.get(), true);
    if (err.isNotEmpty())
        DBG ("Audio init: " << err);

    deviceManager.addAudioCallback (this);
}

void AppState::loadPluginDatabase()
{
    formatManager.addDefaultFormats();

    if (auto xml = settings().getXmlValue ("pluginList"))
        knownPlugins.recreateFromXml (*xml);

    knownPlugins.addChangeListener (this);
}

void AppState::saveSettings()
{
    auto& s = settings();
    s.setValue ("tempo", transport.tempo.load());
    s.setValue ("masterVolume", (double) transport.masterVolume.load());

    if (auto xml = deviceManager.createStateXml())
        s.setValue ("audioDeviceState", xml.get());

    if (auto xml = knownPlugins.createXml())
        s.setValue ("pluginList", xml.get());

    props.saveIfNeeded();
}

void AppState::changeListenerCallback (juce::ChangeBroadcaster*)
{
    saveSettings();
}

void AppState::audioDeviceAboutToStart (juce::AudioIODevice* device)
{
    sampleRate = device->getCurrentSampleRate();
}

void AppState::audioDeviceIOCallbackWithContext (const float* const*, int,
                                                 float* const* out, int numOut,
                                                 int numSamples, const juce::AudioIODeviceCallbackContext&)
{
    // The mixing engine will render here. For now: silence + transport clock.
    for (int ch = 0; ch < numOut; ++ch)
        if (out[ch] != nullptr)
            juce::FloatVectorOperations::clear (out[ch], numSamples);

    if (transport.playing)
    {
        const double ticksPerSecond = transport.tempo.load() / 60.0 * Transport::ppq;
        auto pos = transport.positionTicks.load() + numSamples / sampleRate.load() * ticksPerSecond;

        // PAT mode loops one pattern (1 bar until the sequencer exists).
        if (! transport.songMode)
        {
            const double loopLen = Transport::stepsPerBar * Transport::ticksPerStep;
            pos = std::fmod (pos, loopLen);
        }

        transport.positionTicks = pos;
    }
}
