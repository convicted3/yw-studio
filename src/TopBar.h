#pragma once
#include <JuceHeader.h>
#include "AppState.h"
#include "Widgets.h"

class TempoDisplay;
class TimeDisplay;
class ScopeDisplay;
class PerfDisplay;
class PatSongSwitch;
class SongPositionSlider;
class PatternSelector;
class NewsPanel;

// The two-row main toolbar that also acts as the window caption.
// Row 1: menu, master volume, transport, tempo, recording switches, time, scope, CPU, quick actions, window buttons.
// Row 2: hint bar, master pitch, song position, editing switches, snap, pattern selector, window shortcuts, news.
class TopBar : public juce::Component, private juce::Timer
{
public:
    TopBar (AppState&, juce::MenuBarModel&);
    ~TopBar() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

    void togglePlay();
    void stop();
    void togglePatSong();
    void refresh();

    // Window shortcuts: playlist, pianoRoll, channelRack, mixer, browser, projectInfo, pluginPicker, catalog
    enum Shortcut { scPlaylist, scPianoRoll, scChannelRack, scMixer, scBrowser, scProject, scPluginPicker, scCatalog, numShortcuts };
    std::function<void (int)> onShortcut;
    std::function<bool (int)> isShortcutActive;
    std::function<void (int)> onCommand;   // menu command ids from MenuModel

    static constexpr int height = 78;

private:
    void timerCallback() override;
    void maximiseToggle();

    AppState& state;

    juce::MenuBarComponent menuBar;
    Knob masterVolume { "Master volume", 0.8f };
    Knob masterPitch  { "Master pitch", 0.5f, true };

    std::unique_ptr<PatSongSwitch> patSong;
    ToolButton playButton   { icons::Id::play,   "Play / Pause (Space)" };
    ToolButton stopButton   { icons::Id::stop,   "Stop (Space)" };
    ToolButton recordButton { icons::Id::record, "Record (R)" };
    std::unique_ptr<TempoDisplay> tempo;

    juce::OwnedArray<ToolButton> recSwitches;  // metronome, wait for input, countdown, blend, loop record
    std::unique_ptr<TimeDisplay> time;
    std::unique_ptr<ScopeDisplay> scope;
    std::unique_ptr<PerfDisplay> perf;
    juce::OwnedArray<ToolButton> quickActions; // reset, record audio, help, save, save new version, feedback, catalog

    ToolButton minButton   { icons::Id::minimise, "Minimize", ToolButton::Style::flat };
    ToolButton maxButton   { icons::Id::maximise, "Maximize", ToolButton::Style::flat };
    ToolButton closeButton { icons::Id::close,    "Close",    ToolButton::Style::flat };

    HintArea hintArea;
    juce::String hintTitle { "[YW Studio]" }, hintText;

    std::unique_ptr<SongPositionSlider> songPos;
    juce::OwnedArray<ToolButton> editSwitches; // typing keyboard, step edit, slide, multilink, recording filter
    ToolButton snapIcon { icons::Id::magnet, "Global snap", ToolButton::Style::flat };
    juce::ComboBox snap;
    ToolButton patternMenu { icons::Id::arrowRight, "Pattern options", ToolButton::Style::raised };
    std::unique_ptr<PatternSelector> pattern;
    ToolButton patternAdd { icons::Id::plus, "Find next empty pattern (F4)", ToolButton::Style::flat };
    juce::OwnedArray<ToolButton> shortcuts;
    std::unique_ptr<NewsPanel> news;

    juce::ComponentDragger dragger;
    bool draggingWindow = false;

    // layout anchors, computed in resized()
    juce::Rectangle<int> menuArea, hintBox, row1, row2;
    juce::Array<juce::Rectangle<int>> groupBoxes;
};
