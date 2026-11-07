#pragma once
#include <JuceHeader.h>
#include "SampleLibrary.h"

// Transport position is advanced on the audio thread and read by the UI.
struct Transport
{
    static constexpr int ppq = 96;          // ticks per quarter note
    static constexpr int ticksPerStep = 24; // 1/16
    static constexpr int stepsPerBar = 16;  // 4/4

    std::atomic<bool>   playing  { false };
    std::atomic<bool>   songMode { false }; // false = PAT, true = SONG
    std::atomic<double> tempo    { 130.0 };
    std::atomic<double> positionTicks { 0.0 };
    std::atomic<float>  masterVolume { 0.8f };

    void stop() { playing = false; positionTicks = 0.0; }
};

// Global application state: settings, audio device, plugin database, transport.
class AppState : private juce::AudioIODeviceCallback,
                 private juce::ChangeListener
{
public:
    AppState();
    ~AppState() override;

    // Startup steps, called one by one while the splash is visible.
    void loadSettings();
    void initialiseAudio();
    void loadPluginDatabase();

    void saveSettings();

    juce::PropertiesFile& settings()            { return *props.getUserSettings(); }
    juce::File getAppDataFolder() const;
    juce::File getDeadMansPedalFile() const     { return getAppDataFolder().getChildFile ("plugin-scan.crashcheck"); }

    // Extra folders shown in the browser (packs, drum kits...)
    juce::StringArray getBrowserFolders()      { return juce::StringArray::fromLines (settings().getValue ("browserFolders")); }
    void setBrowserFolders (const juce::StringArray& f) { settings().setValue ("browserFolders", f.joinIntoString ("\n")); saveSettings(); }

    bool getShowSplash()                        { return settings().getBoolValue ("showSplash", true); }
    void setShowSplash (bool b)                 { settings().setValue ("showSplash", b); }

    juce::AudioDeviceManager      deviceManager;
    juce::AudioPluginFormatManager formatManager;
    juce::KnownPluginList         knownPlugins;
    Transport                     transport;
    SampleLibrary                 samples;

    // Project (message thread only; the project model will replace these)
    int numPatterns = 1, currentPattern = 1;

    static constexpr const char* version = JUCE_APPLICATION_VERSION_STRING;

private:
    void audioDeviceIOCallbackWithContext (const float* const* inputChannelData, int numInputChannels,
                                           float* const* outputChannelData, int numOutputChannels,
                                           int numSamples, const juce::AudioIODeviceCallbackContext&) override;
    void audioDeviceAboutToStart (juce::AudioIODevice*) override;
    void audioDeviceStopped() override {}

    void changeListenerCallback (juce::ChangeBroadcaster*) override;

    juce::ApplicationProperties props;
    std::atomic<double> sampleRate { 44100.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AppState)
};
