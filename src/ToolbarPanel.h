#pragma once
#include <JuceHeader.h>
#include "AppState.h"

// Small vector-icon button used across the toolbar.
class IconButton : public juce::Button
{
public:
    enum class Icon { play, stop, record, playlist, pianoRoll, channelRack, mixer, browser, pluginPicker };

    IconButton (const juce::String& name, Icon i) : juce::Button (name), icon (i) {}
    void paintButton (juce::Graphics&, bool over, bool down) override;

private:
    Icon icon;
};

// LCD-style tempo display: drag up/down to change, right-click for options.
class TempoDisplay : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit TempoDisplay (Transport& t);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    void setTempo (double bpm);
    void typeInValue();
    void tap();

    Transport& transport;
    double dragStartTempo = 0.0;
    bool fineDrag = false;
    juce::Array<double> taps;
};

// Song position display (Bar:Step:Tick).
class TimeDisplay : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit TimeDisplay (Transport& t) : transport (t) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
private:
    Transport& transport;
    bool showMinSec = false;
};

// CPU load and voices meter.
class CpuMeter : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit CpuMeter (juce::AudioDeviceManager& dm) : deviceManager (dm) {}
    void paint (juce::Graphics&) override;
private:
    juce::AudioDeviceManager& deviceManager;
};

// Main toolbar: transport, tempo, time, master, snap, pattern, CPU, window shortcuts.
class ToolbarPanel : public juce::Component, private juce::Timer
{
public:
    explicit ToolbarPanel (AppState& s);

    void paint (juce::Graphics&) override;
    void resized() override;

    void togglePlay();
    void stop();
    void togglePatSong();

    // Window shortcut callbacks: 0 playlist, 1 piano roll, 2 channel rack, 3 mixer, 4 browser, 5 plugin picker
    std::function<void (int)> onShowWindow;

private:
    void timerCallback() override;
    void refreshTransportButtons();

    AppState& state;

    juce::TextButton patButton { "PAT" }, songButton { "SONG" };
    IconButton playButton   { "Play",   IconButton::Icon::play };
    IconButton stopButton   { "Stop",   IconButton::Icon::stop };
    IconButton recordButton { "Record", IconButton::Icon::record };

    TempoDisplay tempo;
    TimeDisplay  time;

    juce::Slider masterVolume { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider masterPitch  { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };

    juce::ComboBox snap;
    juce::ComboBox pattern;
    juce::TextButton patternAdd { "+" };

    CpuMeter cpu;

    juce::OwnedArray<IconButton> shortcuts;
};
