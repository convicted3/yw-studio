#include "TopBar.h"
#include "LookAndFeel.h"
#include "MenuModel.h"

#if JUCE_WINDOWS
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <windows.h>
 #include <psapi.h>
 #pragma comment (lib, "psapi.lib")
 #undef small
#endif

using namespace yw;

namespace
{
    void drawInset (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour fill = theme::lcd)
    {
        g.setColour (fill);
        g.fillRoundedRectangle (r, 2.5f);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawRoundedRectangle (r, 2.5f, 1.0f);
    }

    int processMemoryMB()
    {
       #if JUCE_WINDOWS
        PROCESS_MEMORY_COUNTERS pmc {};
        if (GetProcessMemoryInfo (GetCurrentProcess(), &pmc, sizeof (pmc)))
            return (int) (pmc.WorkingSetSize / (1024 * 1024));
       #endif
        return 0;
    }
}

//==============================================================================
class PatSongSwitch : public juce::Component, public juce::TooltipClient
{
public:
    explicit PatSongSwitch (Transport& t) : transport (t) {}
    juce::String getTooltip() override { return "Pattern / Song mode (L)"; }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        const bool song = transport.songMode;
        auto top = r.removeFromTop (r.getHeight() * 0.5f);

        g.setColour (theme::faceDark);
        g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 3.0f);

        auto active = song ? r : top;
        g.setColour (theme::faceActive);
        g.fillRoundedRectangle (active.reduced (1.5f), 2.0f);

        g.setFont (theme::font (13.0f, theme::Weight::semibold));
        g.setColour (song ? theme::textDim : juce::Colour (0xff1e2326));
        g.drawText ("PAT", top, juce::Justification::centred);
        g.setColour (song ? juce::Colour (0xff1e2326) : theme::textDim);
        g.drawText ("SONG", r, juce::Justification::centred);

        g.setColour (theme::outline);
        g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 3.0f, 1.0f);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        transport.songMode = e.y > getHeight() / 2;
        repaint();
    }

private:
    Transport& transport;
};

//==============================================================================
class TempoDisplay : public juce::Component, public juce::TooltipClient
{
public:
    explicit TempoDisplay (Transport& t) : transport (t) { setMouseCursor (juce::MouseCursor::UpDownResizeCursor); }

    juce::String getTooltip() override
    {
        return "Tempo - " + juce::String (transport.tempo.load(), 3) + " BPM (drag, Shift = fine, right-click for options)";
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        drawInset (g, r.reduced (0.5f), juce::Colour (0xff3a4246));

        const double bpm = transport.tempo;
        int whole = (int) bpm, frac = juce::roundToInt ((bpm - whole) * 1000.0);
        if (frac == 1000) { ++whole; frac = 0; }

        auto inner = getLocalBounds().reduced (4, 0).withTrimmedRight (8);
        juce::GlyphArrangement ga;
        const auto bigF = theme::font (22.0f), smallF = theme::font (14.0f);
        const auto bigS = juce::String (whole), smallS = "." + juce::String (frac).paddedLeft ('0', 3);
        const float wSmall = bigF.getHeight() > 0 ? juce::GlyphArrangement::getStringWidth (smallF, smallS) : 0.0f;

        g.setColour (theme::textBright);
        g.setFont (bigF);
        g.drawText (bigS, inner.withTrimmedRight ((int) wSmall + 1), juce::Justification::centredRight);
        g.setColour (theme::text);
        g.setFont (smallF);
        g.drawText (smallS, inner.withTrimmedTop (5), juce::Justification::centredRight);

        // up/down hint arrows
        const float ax = (float) getWidth() - 6.0f;
        juce::Path p;
        p.addTriangle (ax - 2.5f, 11.0f, ax + 2.5f, 11.0f, ax, 7.5f);
        p.addTriangle (ax - 2.5f, getHeight() - 11.0f, ax + 2.5f, getHeight() - 11.0f, ax, getHeight() - 7.5f);
        g.setColour (theme::textDim);
        g.fillPath (p);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu()) { showMenu(); return; }
        dragStart = transport.tempo;
        fine = e.mods.isShiftDown() || e.x > getWidth() * 0.58f;
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu()) return;
        const double d = -e.getDistanceFromDragStartY();
        set (fine ? dragStart + d * 0.01 : std::round (dragStart + d * 0.3));
    }

    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override
    {
        set (transport.tempo + (w.deltaY > 0 ? 1.0 : -1.0) * (e.mods.isShiftDown() ? 0.01 : 1.0));
    }

    void mouseDoubleClick (const juce::MouseEvent&) override { typeIn(); }

private:
    void set (double v) { transport.tempo = juce::jlimit (10.0, 522.0, std::round (v * 1000.0) / 1000.0); repaint(); }

    void showMenu()
    {
        juce::PopupMenu m, presets;
        m.addItem (1, "Type in value...");
        m.addItem (2, "Tap tempo");
        for (int b : { 80, 90, 100, 110, 120, 130, 140, 150, 160 }) presets.addItem (100 + b, juce::String (b));
        m.addSubMenu ("Presets", presets);
        m.addSeparator();
        m.addItem (3, "Copy value");
        m.addItem (4, "Paste value");
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [this] (int r)
        {
            if (r == 1) typeIn();
            else if (r == 2) tap();
            else if (r == 3) juce::SystemClipboard::copyTextToClipboard (juce::String (transport.tempo.load(), 3));
            else if (r == 4) { auto v = juce::SystemClipboard::getTextFromClipboard().getDoubleValue(); if (v > 0) set (v); }
            else if (r > 100) set (r - 100);
        });
    }

    void typeIn()
    {
        auto* w = new juce::AlertWindow ("Tempo", "Enter tempo (10 - 522 BPM):", juce::MessageBoxIconType::NoIcon, this);
        w->addTextEditor ("bpm", juce::String (transport.tempo.load(), 3));
        w->addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey));
        w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
        w->enterModalState (true, juce::ModalCallbackFunction::create ([this, w] (int r)
        { if (r == 1) set (w->getTextEditorContents ("bpm").getDoubleValue()); }), true);
    }

    void tap()
    {
        const auto now = juce::Time::getMillisecondCounterHiRes();
        if (! taps.isEmpty() && now - taps.getLast() > 2000.0) taps.clear();
        taps.add (now);
        if (taps.size() > 8) taps.remove (0);
        if (taps.size() >= 2) set (60000.0 * (taps.size() - 1) / (taps.getLast() - taps.getFirst()));
    }

    Transport& transport;
    double dragStart = 0.0;
    bool fine = false;
    juce::Array<double> taps;
};

//==============================================================================
class TimeDisplay : public juce::Component, public juce::TooltipClient
{
public:
    explicit TimeDisplay (Transport& t) : transport (t) {}
    juce::String getTooltip() override { return "Song position - click to switch time format"; }

    void paint (juce::Graphics& g) override
    {
        const double ticks = transport.positionTicks;
        juce::String big, smallText, unit;

        if (! barsMode)
        {
            const double sec = ticks / Transport::ppq * 60.0 / transport.tempo.load();
            big   = juce::String ((int) (sec / 60.0)) + ":" + juce::String ((int) std::fmod (sec, 60.0)).paddedLeft ('0', 2);
            smallText = ":" + juce::String ((int) (std::fmod (sec, 1.0) * 100.0)).paddedLeft ('0', 2);
            unit  = "M:S:CS";
        }
        else
        {
            const int t = (int) ticks;
            big   = juce::String (t / (Transport::ticksPerStep * Transport::stepsPerBar) + 1) + ":"
                  + juce::String ((t / Transport::ticksPerStep) % Transport::stepsPerBar + 1).paddedLeft ('0', 2);
            smallText = ":" + juce::String (t % Transport::ticksPerStep).paddedLeft ('0', 2);
            unit  = "B:S:T";
        }

        auto r = getLocalBounds().reduced (6, 0);
        g.setColour (theme::textDim);
        g.setFont (theme::font (11.0f, theme::Weight::semibold));
        g.drawText (unit, r.withHeight (12), juce::Justification::topRight);

        const auto bigF = theme::font (40.0f), smallF = theme::font (26.0f);
        const float smallW = juce::GlyphArrangement::getStringWidth (smallF, smallText);
        g.setColour (theme::lcdText);
        g.setFont (smallF);
        g.drawText (smallText, r.withTrimmedTop (9), juce::Justification::centredRight);
        g.setFont (bigF);
        g.drawText (big, r.withTrimmedRight ((int) smallW).withTrimmedTop (1), juce::Justification::centredRight);
    }

    void mouseDown (const juce::MouseEvent&) override { barsMode = ! barsMode; repaint(); }

private:
    Transport& transport;
    bool barsMode = false;
};

//==============================================================================
class ScopeDisplay : public juce::Component, public juce::TooltipClient
{
public:
    juce::String getTooltip() override { return "Output oscilloscope"; }
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        drawInset (g, r.reduced (0.5f), juce::Colour (0xff1c2225));
        g.setColour (theme::green.withAlpha (0.55f));
        g.drawHorizontalLine (getHeight() / 2, 4.0f, r.getRight() - 4.0f);
        icons::draw (g, icons::Id::eye, { r.getRight() - 22.0f, r.getBottom() - 15.0f, 16.0f, 13.0f }, theme::textDim);
    }
};

//==============================================================================
class PerfDisplay : public juce::Component, public juce::TooltipClient
{
public:
    explicit PerfDisplay (juce::AudioDeviceManager& dm) : deviceManager (dm) {}
    juce::String getTooltip() override
    {
        return "CPU " + juce::String (juce::roundToInt (cpu * 100.0)) + "%  -  Memory " + juce::String (mem) + " MB  -  Polyphony 0";
    }

    void update()
    {
        cpu = juce::jlimit (0.0, 1.0, deviceManager.getCpuUsage());
        mem = processMemoryMB();
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().reduced (4, 2);
        auto right = r.removeFromRight (r.getWidth() / 2);

        // CPU meter (left)
        g.setColour (theme::textBright);
        g.fillRect (r.getX(), r.getY() + 2, 2, r.getHeight() / 2 - 4);
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.fillRect (r.getX(), r.getBottom() - 4, r.getWidth() - 4, 1);
        g.setColour (theme::text);
        g.fillRect (r.getX(), r.getBottom() - 4, juce::roundToInt ((r.getWidth() - 4) * cpu), 1);
        g.setFont (theme::font (15.0f));
        g.drawText (juce::String (juce::roundToInt (cpu * 100.0)), r.withTrimmedBottom (r.getHeight() / 2), juce::Justification::centredRight);

        // memory / polyphony (right)
        g.setColour (theme::text);
        g.drawText (juce::String (mem) + " MB", right.removeFromTop (right.getHeight() / 2), juce::Justification::centredRight);
        g.drawText ("0", right.withTrimmedRight (18), juce::Justification::centredRight);
        icons::draw (g, icons::Id::keys, right.removeFromRight (16).toFloat().reduced (1.0f, 3.0f), theme::textDim);
    }

private:
    juce::AudioDeviceManager& deviceManager;
    double cpu = 0.0;
    int mem = 0;
};

//==============================================================================
class SongPositionSlider : public juce::Component, public juce::TooltipClient
{
public:
    explicit SongPositionSlider (Transport& t) : transport (t) {}
    juce::String getTooltip() override { return "Song position"; }

    static constexpr double lengthTicks = 16.0 * Transport::stepsPerBar * Transport::ticksPerStep; // 16 bars

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        const float cy = r.getCentreY();
        g.setColour (juce::Colour (0xff1c2225));
        g.fillRoundedRectangle (r.getX() + 8.0f, cy - 1.5f, r.getWidth() - 16.0f, 3.0f, 1.5f);

        const float x = 8.0f + (float) juce::jlimit (0.0, 1.0, transport.positionTicks.load() / lengthTicks) * (r.getWidth() - 16.0f);
        juce::Rectangle<float> thumb (x - 6.0f, cy - 9.0f, 12.0f, 18.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4c5559), 0, thumb.getY(), juce::Colour (0xff23292c), 0, thumb.getBottom(), false));
        g.fillEllipse (thumb.withSizeKeepingCentre (16.0f, 16.0f));
        g.setColour (theme::outline);
        g.drawEllipse (thumb.withSizeKeepingCentre (16.0f, 16.0f), 1.0f);
        g.setColour (theme::textBright);
        g.fillRect (x - 0.75f, cy - 4.0f, 1.5f, 8.0f);
    }

    void mouseDown (const juce::MouseEvent& e) override { mouseDrag (e); }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        const double p = juce::jlimit (0.0, 1.0, (e.position.x - 8.0) / juce::jmax (1.0, getWidth() - 16.0));
        transport.positionTicks = p * lengthTicks;
        repaint();
    }

private:
    Transport& transport;
};

//==============================================================================
class PatternSelector : public juce::Component, public juce::TooltipClient
{
public:
    explicit PatternSelector (AppState& s) : state (s) {}
    juce::String getTooltip() override { return "Pattern selector - click for list, drag or wheel to change"; }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff5a6266), 0, r.getY(), juce::Colour (0xff3d4448), 0, r.getBottom(), false));
        g.fillRoundedRectangle (r, 2.5f);
        g.setColour (theme::outline);
        g.drawRoundedRectangle (r, 2.5f, 1.0f);

        g.setColour (theme::textBright);
        g.setFont (theme::font (22.0f));
        g.drawText ("Pattern " + juce::String (state.currentPattern), getLocalBounds().withTrimmedRight (14),
                    juce::Justification::centredRight);

        const float ax = (float) getWidth() - 7.0f;
        juce::Path p;
        p.addTriangle (ax - 2.5f, 10.0f, ax + 2.5f, 10.0f, ax, 6.5f);
        p.addTriangle (ax - 2.5f, getHeight() - 10.0f, ax + 2.5f, getHeight() - 10.0f, ax, getHeight() - 6.5f);
        g.setColour (theme::text);
        g.fillPath (p);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        dragStart = state.currentPattern;
        if (e.mods.isPopupMenu() || e.x < getWidth() - 16)
        {
            juce::PopupMenu m;
            for (int i = 1; i <= state.numPatterns; ++i)
                m.addItem (i, "Pattern " + juce::String (i), true, i == state.currentPattern);
            m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [this] (int r)
            { if (r > 0) { state.currentPattern = r; repaint(); } });
        }
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        state.currentPattern = juce::jlimit (1, 999, dragStart - e.getDistanceFromDragStartY() / 8);
        state.numPatterns = juce::jmax (state.numPatterns, state.currentPattern);
        repaint();
    }

    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override
    {
        state.currentPattern = juce::jlimit (1, 999, state.currentPattern + (w.deltaY > 0 ? 1 : -1));
        state.numPatterns = juce::jmax (state.numPatterns, state.currentPattern);
        repaint();
    }

private:
    AppState& state;
    int dragStart = 1;
};

//==============================================================================
class NewsPanel : public juce::Component, public juce::TooltipClient
{
public:
    juce::String getTooltip() override { return "YW news - click to open yolo.su"; }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        drawInset (g, r.reduced (0.5f), juce::Colour (0xff2a3335));
        auto t = getLocalBounds().reduced (8, 3).withTrimmedRight (30);
        auto l1 = t.removeFromTop (t.getHeight() / 2);
        g.setFont (theme::font (16.0f));
        g.setColour (theme::textDim);
        g.drawText ("Today", l1, juce::Justification::centredLeft);
        g.setColour (theme::text);
        g.drawText ("Welcome to YW Studio", l1.withTrimmedLeft (44), juce::Justification::centredLeft, true);
        g.drawText ("0.1 alpha by #YW - yolo.su", t, juce::Justification::centredLeft, true);
        icons::draw (g, icons::Id::globe, { r.getRight() - 28.0f, r.getCentreY() - 10.0f, 20.0f, 20.0f }, theme::text);
    }

    void mouseUp (const juce::MouseEvent&) override { juce::URL ("https://yolo.su").launchInDefaultBrowser(); }
};

//==============================================================================
TopBar::TopBar (AppState& s, juce::MenuBarModel& model)
    : state (s), menuBar (&model)
{
    addAndMakeVisible (menuBar);

    masterVolume.setStyle (true);
    masterVolume.setValue (state.transport.masterVolume, false);
    masterVolume.onChange = [this] (float v) { state.transport.masterVolume = v; };
    masterVolume.setDisplay ([] (float v) { return v <= 0.0001f ? juce::String ("-inf dB")
                                                             : juce::String (juce::Decibels::gainToDecibels (v / 0.8f), 1) + " dB"; });
    addAndMakeVisible (masterVolume);

    masterPitch.setStyle (true);
    masterPitch.setDisplay ([] (float v) { return juce::String (juce::roundToInt ((v - 0.5f) * 2400.0f)) + " cents"; });
    addAndMakeVisible (masterPitch);

    patSong = std::make_unique<PatSongSwitch> (state.transport);
    addAndMakeVisible (*patSong);

    playButton.onClick = [this] { togglePlay(); };
    stopButton.onClick = [this] { stop(); };
    recordButton.setClickingTogglesState (true);
    for (auto* b : { &playButton, &stopButton, &recordButton }) addAndMakeVisible (b);

    tempo = std::make_unique<TempoDisplay> (state.transport);
    addAndMakeVisible (*tempo);

    struct Def { icons::Id icon; const char* hint; bool on; };
    const Def rec[] = {
        { icons::Id::metronome,  "Metronome (Ctrl+M)", false },
        { icons::Id::waitInput,  "Wait for input to start recording (Ctrl+I)", false },
        { icons::Id::countdown,  "Countdown before recording (Ctrl+P)", true },
        { icons::Id::blend,      "Blend recorded notes (Ctrl+B)", false },
        { icons::Id::loopRecord, "Loop recording", false } };
    for (auto& d : rec)
    {
        auto* b = recSwitches.add (new ToolButton (d.icon, d.hint));
        b->setClickingTogglesState (true);
        b->setToggleState (d.on, juce::dontSendNotification);
        addAndMakeVisible (b);
    }

    time  = std::make_unique<TimeDisplay> (state.transport);
    scope = std::make_unique<ScopeDisplay>();
    perf  = std::make_unique<PerfDisplay> (state.deviceManager);
    addAndMakeVisible (*time);
    addAndMakeVisible (*scope);
    addAndMakeVisible (*perf);

    struct Act { icons::Id icon; const char* hint; int command; };
    const Act acts[] = {
        { icons::Id::reset,    "Undo (Ctrl+Z)",                 cmd::editUndo },
        { icons::Id::slice,    "Cut (Ctrl+X)",                  cmd::editCut },
        { icons::Id::mic,      "One-click audio recording",     cmd::toolOneClickRecord },
        { icons::Id::help,     "Help (F1)",                     cmd::helpIndex },
        { icons::Id::save,     "Save (Ctrl+S)",                 cmd::fileSave },
        { icons::Id::saveNew,  "Save new version (Ctrl+N)",     cmd::fileSaveNewVersion },
        { icons::Id::chat,     "Feedback and bug reports (GitHub)", cmd::helpGithub },
        { icons::Id::download, "YW plugin catalog",             cmd::toolPluginCatalog } };
    for (auto& a : acts)
    {
        auto* b = quickActions.add (new ToolButton (a.icon, a.hint));
        const int c = a.command;
        b->onClick = [this, c] { if (onCommand) onCommand (c); };
        addAndMakeVisible (b);
    }

    minButton.onClick   = [this] { if (auto* w = dynamic_cast<juce::ResizableWindow*> (getTopLevelComponent())) w->setMinimised (true); };
    maxButton.onClick   = [this] { maximiseToggle(); };
    closeButton.onClick = [] { juce::JUCEApplication::getInstance()->systemRequestedQuit(); };
    for (auto* b : { &minButton, &maxButton, &closeButton }) { b->setIconScale (0.9f); addAndMakeVisible (b); }

    hintArea.setTooltip ("Hint bar - shows information about the control under the mouse");
    addAndMakeVisible (hintArea);

    songPos = std::make_unique<SongPositionSlider> (state.transport);
    addAndMakeVisible (*songPos);

    const Def edit[] = {
        { icons::Id::typingKeyboard, "Typing keyboard to piano keyboard (Ctrl+T)", true },
        { icons::Id::stepEdit,       "Step editing mode (Ctrl+E)", false },
        { icons::Id::slide,          "Slide notes", false },
        { icons::Id::link,           "Multilink to controllers (Ctrl+J)", true },
        { icons::Id::recFilter,      "Recording filter", false } };
    for (auto& d : edit)
    {
        auto* b = editSwitches.add (new ToolButton (d.icon, d.hint));
        b->setClickingTogglesState (true);
        b->setToggleState (d.on, juce::dontSendNotification);
        addAndMakeVisible (b);
    }

    addAndMakeVisible (snapIcon);
    int id = 1;
    for (auto t : { "Line", "Cell", "(none)" }) snap.addItem (t, id++);
    snap.addSeparator();
    for (auto t : { "1/6 step", "1/4 step", "1/3 step", "1/2 step", "Step" }) snap.addItem (t, id++);
    snap.addSeparator();
    for (auto t : { "1/6 beat", "1/4 beat", "1/3 beat", "1/2 beat", "Beat" }) snap.addItem (t, id++);
    snap.addSeparator();
    snap.addItem ("Bar", id++);
    snap.setSelectedId (1, juce::dontSendNotification);
    snap.setTooltip ("Global snap");
    addAndMakeVisible (snap);

    patternMenu.onClick = [this]
    {
        juce::PopupMenu m;
        m.addItem (1, "Find next empty pattern  F4");
        m.addItem (2, "Rename and color...  F2", false);
        m.addItem (3, "Clone  Alt+C", false);
        m.addItem (4, "Delete", false);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&patternMenu), [this] (int r)
        { if (r == 1) patternAdd.triggerClick(); });
    };
    addAndMakeVisible (patternMenu);
    pattern = std::make_unique<PatternSelector> (state);
    addAndMakeVisible (*pattern);
    patternAdd.onClick = [this] { state.numPatterns = state.numPatterns + 1; state.currentPattern = state.numPatterns; pattern->repaint(); };
    addAndMakeVisible (patternAdd);

    const Def sc[] = {
        { icons::Id::playlist,    "Playlist (F5)", false },
        { icons::Id::pianoRoll,   "Piano roll (F7)", false },
        { icons::Id::channelRack, "Channel rack (F6)", false },
        { icons::Id::mixer,       "Mixer (F9)", false },
        { icons::Id::browser,     "Browser (Alt+F8)", false },
        { icons::Id::project,     "Project info (F11)", false },
        { icons::Id::plug,        "Plugin picker (F8)", false },
        { icons::Id::catalog,     "YW plugin catalog", false } };
    for (int i = 0; i < (int) std::size (sc); ++i)
    {
        auto* b = shortcuts.add (new ToolButton (sc[i].icon, sc[i].hint));
        b->onClick = [this, i] { if (onShortcut) onShortcut (i); refresh(); };
        addAndMakeVisible (b);
    }

    news = std::make_unique<NewsPanel>();
    addAndMakeVisible (*news);

    startTimerHz (30);
}

TopBar::~TopBar() = default;

void TopBar::togglePlay()
{
    state.transport.playing = ! state.transport.playing.load();
    refresh();
}

void TopBar::stop()
{
    state.transport.stop();
    refresh();
}

void TopBar::togglePatSong()
{
    state.transport.songMode = ! state.transport.songMode.load();
    refresh();
}

void TopBar::refresh()
{
    playButton.setToggleState (state.transport.playing, juce::dontSendNotification);
    playButton.setIcon (state.transport.playing ? icons::Id::listen : icons::Id::play);
    patSong->repaint();
    for (int i = 0; i < shortcuts.size(); ++i)
        shortcuts[i]->setToggleState (isShortcutActive && isShortcutActive (i), juce::dontSendNotification);
}

void TopBar::timerCallback()
{
    time->repaint();
    songPos->repaint();
    if (playButton.getToggleState() != state.transport.playing.load()) refresh();

    static int n = 0;
    if (++n % 15 == 0) perf->update();

    // Hint bar: tooltip of whatever is under the mouse
    juce::String h;
    auto* c = juce::Desktop::getInstance().getMainMouseSource().getComponentUnderMouse();
    for (; c != nullptr; c = c->getParentComponent())
        if (auto* tc = dynamic_cast<juce::TooltipClient*> (c))
            if ((h = tc->getTooltip()).isNotEmpty()) break;
    if (h.isEmpty()) h = "Welcome to YW Studio - free DAW by #YW";
    if (h != hintText) { hintText = h; repaint (hintBox); }
}

void TopBar::paint (juce::Graphics& g)
{
    g.fillAll (theme::topBar);

    // menu area is slightly tinted like a caption
    g.setColour (juce::Colour (0xff2f3c3d));
    g.fillRect (menuArea);

    // grouped panels (darker frames behind button groups)
    for (auto& b : groupBoxes)
    {
        g.setColour (juce::Colour (0xff272e31));
        g.fillRoundedRectangle (b.toFloat(), 3.0f);
    }

    // hint panel
    drawInset (g, hintBox.toFloat(), theme::hintBg);
    auto t = hintBox.reduced (6, 3);
    g.setFont (theme::font (15.0f));
    g.setColour (theme::textDim);
    g.drawText (hintTitle, t.removeFromTop (t.getHeight() / 2), juce::Justification::centredLeft, true);
    g.setColour (theme::text);
    g.drawText (hintText, t, juce::Justification::centredLeft, true);

    // separators between the two rows
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.drawHorizontalLine (getHeight() - 1, 0.0f, (float) getWidth());
}

void TopBar::resized()
{
    groupBoxes.clear();
    auto r = getLocalBounds();
    row1 = r.removeFromTop (34);
    r.removeFromTop (2);
    row2 = r.removeFromTop (40);

    const int leftW = 300;
    // Row 1 ---------------------------------------------------------------
    auto a = row1;
    menuArea = a.removeFromLeft (leftW);
    menuBar.setBounds (menuArea.reduced (2, 1));
    a.removeFromLeft (4);
    masterVolume.setBounds (a.removeFromLeft (32).reduced (1));
    a.removeFromLeft (10);

    auto transportBox = a.removeFromLeft (44 + 3 * 32 + 10).reduced (0, 1);
    groupBoxes.add (transportBox);
    {
        auto tb = transportBox.reduced (2);
        patSong->setBounds (tb.removeFromLeft (44));
        tb.removeFromLeft (3);
        playButton.setBounds (tb.removeFromLeft (30));
        tb.removeFromLeft (2);
        stopButton.setBounds (tb.removeFromLeft (30));
        tb.removeFromLeft (4);
        recordButton.setBounds (tb.removeFromLeft (30));
    }
    a.removeFromLeft (8);
    tempo->setBounds (a.removeFromLeft (72).reduced (0, 3));
    a.removeFromLeft (12);
    for (auto* b : recSwitches) { b->setBounds (a.removeFromLeft (32).reduced (1)); a.removeFromLeft (1); }
    a.removeFromLeft (4);
    time->setBounds (a.removeFromLeft (132));
    a.removeFromLeft (6);
    scope->setBounds (a.removeFromLeft (128).reduced (0, 1));
    a.removeFromLeft (6);
    perf->setBounds (a.removeFromLeft (150).reduced (0, 1));
    a.removeFromLeft (8);
    for (auto* b : quickActions) { b->setBounds (a.removeFromLeft (32).reduced (1)); a.removeFromLeft (1); }

    closeButton.setBounds (row1.getRight() - 30, 4, 26, 24);
    maxButton.setBounds (closeButton.getX() - 28, 4, 26, 24);
    minButton.setBounds (maxButton.getX() - 28, 4, 26, 24);

    // Row 2 ---------------------------------------------------------------
    auto b = row2;
    hintBox = b.removeFromLeft (leftW).reduced (0, 1);
    hintArea.setBounds (hintBox);
    b.removeFromLeft (4);
    masterPitch.setBounds (b.removeFromLeft (32).reduced (1, 3));
    b.removeFromLeft (10);
    songPos->setBounds (b.removeFromLeft (184));
    b.removeFromLeft (12);
    for (auto* e : editSwitches) { e->setBounds (b.removeFromLeft (32).reduced (1, 3)); b.removeFromLeft (1); }
    b.removeFromLeft (6);
    snapIcon.setBounds (b.removeFromLeft (22).reduced (0, 9));
    snap.setBounds (b.removeFromLeft (92).reduced (0, 7));
    b.removeFromLeft (12);
    patternMenu.setBounds (b.removeFromLeft (20).reduced (0, 5));
    b.removeFromLeft (2);
    pattern->setBounds (b.removeFromLeft (150).reduced (0, 4));
    patternAdd.setBounds (b.removeFromLeft (24).reduced (0, 7));
    b.removeFromLeft (8);
    for (auto* s : shortcuts) { s->setBounds (b.removeFromLeft (34).reduced (1, 2)); b.removeFromLeft (1); }
    b.removeFromLeft (10);
    news->setBounds (b.removeFromLeft (juce::jmin (300, b.getWidth())).reduced (0, 1));
}

void TopBar::mouseDown (const juce::MouseEvent& e)
{
    draggingWindow = false;
    if (auto* w = getTopLevelComponent())
    {
        if (auto* rw = dynamic_cast<juce::ResizableWindow*> (w); rw != nullptr && rw->isFullScreen()) return;
        dragger.startDraggingComponent (w, e.getEventRelativeTo (w));
        draggingWindow = true;
    }
}

void TopBar::mouseDrag (const juce::MouseEvent& e)
{
    if (draggingWindow)
        if (auto* w = getTopLevelComponent())
            dragger.dragComponent (w, e.getEventRelativeTo (w), nullptr);
}

void TopBar::mouseDoubleClick (const juce::MouseEvent&) { maximiseToggle(); }

void TopBar::maximiseToggle()
{
    if (auto* w = dynamic_cast<juce::ResizableWindow*> (getTopLevelComponent()))
        w->setFullScreen (! w->isFullScreen());
}
