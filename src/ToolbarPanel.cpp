#include "ToolbarPanel.h"
#include "LookAndFeel.h"

using namespace yw;

//==============================================================================
void IconButton::paintButton (juce::Graphics& g, bool over, bool down)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    const bool on = getToggleState();

    g.setColour (on ? theme::accentSoft : (down ? theme::bgControl.brighter (0.15f)
                                                : over ? theme::bgControl.brighter (0.07f) : theme::bgControl));
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (on ? theme::accent : theme::outline);
    g.drawRoundedRectangle (r, 3.0f, 1.0f);

    auto c = r.reduced (r.getHeight() * 0.28f);
    const auto ink = on ? juce::Colours::white : theme::text;
    g.setColour (ink);

    juce::Path p;
    switch (icon)
    {
        case Icon::play:
            p.addTriangle (c.getX() + c.getWidth() * 0.18f, c.getY(),
                           c.getX() + c.getWidth() * 0.18f, c.getBottom(),
                           c.getRight(), c.getCentreY());
            g.setColour (on ? juce::Colour (0xff6ee87a) : ink);
            g.fillPath (p);
            break;
        case Icon::stop:
            g.fillRect (c.reduced (c.getWidth() * 0.1f));
            break;
        case Icon::record:
            g.setColour (on ? theme::accent.brighter (0.3f) : theme::accent);
            g.fillEllipse (c.reduced (c.getWidth() * 0.05f));
            break;
        case Icon::playlist:
            for (int i = 0; i < 3; ++i)
                g.fillRect (c.getX() + (i % 2) * c.getWidth() * 0.3f, c.getY() + i * c.getHeight() * 0.38f,
                            c.getWidth() * 0.7f, c.getHeight() * 0.22f);
            break;
        case Icon::pianoRoll:
            g.drawRect (c, 1.2f);
            for (int i = 1; i < 4; ++i)
                g.fillRect (c.getX() + i * c.getWidth() / 4.0f - 1.0f, c.getY(), 2.0f, c.getHeight() * 0.6f);
            break;
        case Icon::channelRack:
            for (int row = 0; row < 3; ++row)
                for (int col = 0; col < 4; ++col)
                    if ((row + col) % 2 == 0)
                        g.fillRect (c.getX() + col * c.getWidth() / 4.0f, c.getY() + row * c.getHeight() / 3.0f,
                                    c.getWidth() / 4.0f - 1.5f, c.getHeight() / 3.0f - 1.5f);
                    else
                        g.drawRect (c.getX() + col * c.getWidth() / 4.0f, c.getY() + row * c.getHeight() / 3.0f,
                                    c.getWidth() / 4.0f - 1.5f, c.getHeight() / 3.0f - 1.5f, 1.0f);
            break;
        case Icon::mixer:
            for (int i = 0; i < 3; ++i)
            {
                auto x = c.getX() + (i + 0.5f) * c.getWidth() / 3.0f;
                g.drawLine (x, c.getY(), x, c.getBottom(), 1.2f);
                g.fillRect (x - 3.0f, c.getY() + c.getHeight() * (0.2f + 0.25f * i), 6.0f, 3.0f);
            }
            break;
        case Icon::browser:
            g.drawRect (c, 1.2f);
            g.fillRect (c.withWidth (c.getWidth() * 0.35f));
            break;
        case Icon::pluginPicker:
            g.drawEllipse (c.reduced (1.0f), 1.4f);
            g.fillEllipse (c.reduced (c.getWidth() * 0.32f));
            break;
    }
}

//==============================================================================
TempoDisplay::TempoDisplay (Transport& t) : transport (t)
{
    setTooltip ("Tempo (BPM). Drag up/down, Shift for fine, right-click for options");
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
}

void TempoDisplay::setTempo (double bpm)
{
    transport.tempo = juce::jlimit (10.0, 522.0, std::round (bpm * 1000.0) / 1000.0);
    repaint();
}

void TempoDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (theme::lcd);
    g.fillRoundedRectangle (r, 3.0f);

    auto bpm = transport.tempo.load();
    auto whole = (int) bpm;
    auto frac = juce::roundToInt ((bpm - whole) * 1000.0);
    if (frac == 1000) { ++whole; frac = 0; }

    auto inner = getLocalBounds().reduced (6, 2);
    g.setColour (theme::lcdText);
    g.setFont (theme::mono (22.0f));
    auto big = inner.removeFromLeft (juce::roundToInt (inner.getWidth() * 0.62f));
    g.drawText (juce::String (whole), big, juce::Justification::centredRight);

    g.setColour (theme::textDim);
    g.setFont (theme::mono (13.0f));
    g.drawText ("." + juce::String (frac).paddedLeft ('0', 3), inner.withTrimmedTop (6), juce::Justification::centredLeft);
}

void TempoDisplay::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        juce::PopupMenu m;
        m.addItem (1, "Type in value...");
        m.addItem (2, "Tap tempo");
        m.addSeparator();
        juce::PopupMenu presets;
        for (int bpm : { 80, 90, 100, 110, 120, 130, 140, 150, 160 })
            presets.addItem (100 + bpm, juce::String (bpm));
        m.addSubMenu ("Presets", presets);
        m.addSeparator();
        m.addItem (3, "Copy value");
        m.addItem (4, "Paste value");

        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [this] (int r)
        {
            if (r == 1) typeInValue();
            else if (r == 2) tap();
            else if (r == 3) juce::SystemClipboard::copyTextToClipboard (juce::String (transport.tempo.load(), 3));
            else if (r == 4) { auto v = juce::SystemClipboard::getTextFromClipboard().getDoubleValue(); if (v > 0) setTempo (v); }
            else if (r > 100) setTempo (r - 100);
        });
        return;
    }

    dragStartTempo = transport.tempo;
    fineDrag = e.mods.isShiftDown() || e.x > getWidth() * 0.62f; // dragging the decimals = fine
}

void TempoDisplay::mouseDrag (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu()) return;
    const double step = fineDrag ? 0.01 : 0.25;
    setTempo (fineDrag ? dragStartTempo - e.getDistanceFromDragStartY() * step
                       : std::round (dragStartTempo - e.getDistanceFromDragStartY() * step));
}

void TempoDisplay::mouseDoubleClick (const juce::MouseEvent&) { typeInValue(); }

void TempoDisplay::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    setTempo (transport.tempo + (w.deltaY > 0 ? 1.0 : -1.0) * (e.mods.isShiftDown() ? 0.01 : 1.0));
}

void TempoDisplay::typeInValue()
{
    auto* w = new juce::AlertWindow ("Tempo", "Enter tempo (10 - 522 BPM):", juce::MessageBoxIconType::NoIcon, this);
    w->addTextEditor ("bpm", juce::String (transport.tempo.load(), 3));
    w->addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    w->enterModalState (true, juce::ModalCallbackFunction::create ([this, w] (int r)
    {
        if (r == 1) setTempo (w->getTextEditorContents ("bpm").getDoubleValue());
    }), true);
}

void TempoDisplay::tap()
{
    auto now = juce::Time::getMillisecondCounterHiRes();
    if (! taps.isEmpty() && now - taps.getLast() > 2000.0) taps.clear();
    taps.add (now);
    if (taps.size() > 8) taps.remove (0);
    if (taps.size() >= 2)
        setTempo (60000.0 * (taps.size() - 1) / (taps.getLast() - taps.getFirst()));
}

//==============================================================================
void TimeDisplay::paint (juce::Graphics& g)
{
    g.setColour (theme::lcd);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 3.0f);

    const double ticks = transport.positionTicks.load();
    juce::String txt, unit;

    if (showMinSec)
    {
        const double seconds = ticks / Transport::ppq * 60.0 / transport.tempo.load();
        const int m = (int) (seconds / 60.0), s = (int) std::fmod (seconds, 60.0);
        const int cs = (int) (std::fmod (seconds, 1.0) * 100.0);
        txt  = juce::String (m) + ":" + juce::String (s).paddedLeft ('0', 2) + ":" + juce::String (cs).paddedLeft ('0', 2);
        unit = "M:S:CS";
    }
    else
    {
        const int t = (int) ticks;
        const int bar  = t / (Transport::ticksPerStep * Transport::stepsPerBar) + 1;
        const int step = (t / Transport::ticksPerStep) % Transport::stepsPerBar + 1;
        const int tick = t % Transport::ticksPerStep;
        txt  = juce::String (bar) + ":" + juce::String (step).paddedLeft ('0', 2) + ":" + juce::String (tick).paddedLeft ('0', 2);
        unit = "B:S:T";
    }

    auto r = getLocalBounds().reduced (6, 2);
    g.setColour (theme::textDim);
    g.setFont (theme::font (9.0f, true));
    g.drawText (unit, r.removeFromTop (11), juce::Justification::centredLeft);
    g.setColour (theme::lcdText);
    g.setFont (theme::mono (18.0f));
    g.drawText (txt, r, juce::Justification::centredLeft);
}

void TimeDisplay::mouseDown (const juce::MouseEvent&)
{
    showMinSec = ! showMinSec;
    repaint();
}

//==============================================================================
void CpuMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().reduced (2);
    g.setColour (theme::lcd);
    g.fillRoundedRectangle (r.toFloat(), 3.0f);

    const double cpu = juce::jlimit (0.0, 1.0, deviceManager.getCpuUsage());
    auto inner = r.reduced (5, 4);

    g.setColour (theme::textDim);
    g.setFont (theme::font (9.0f, true));
    g.drawText ("CPU", inner.removeFromTop (10), juce::Justification::centredLeft);

    auto bar = inner.removeFromBottom (5).toFloat();
    g.setColour (theme::bgControl);
    g.fillRect (bar);
    g.setColour (cpu > 0.85 ? theme::accent : juce::Colour (0xff6ee87a));
    g.fillRect (bar.withWidth (bar.getWidth() * (float) cpu));

    g.setColour (theme::lcdText);
    g.setFont (theme::mono (12.0f));
    g.drawText (juce::String (juce::roundToInt (cpu * 100.0)) + "%", inner, juce::Justification::centredLeft);
}

//==============================================================================
ToolbarPanel::ToolbarPanel (AppState& s)
    : state (s), tempo (s.transport), time (s.transport), cpu (s.deviceManager)
{
    patButton.setClickingTogglesState (false);
    songButton.setClickingTogglesState (false);
    patButton.setTooltip ("Pattern mode (L)");
    songButton.setTooltip ("Song mode (L)");
    patButton.onClick  = [this] { state.transport.songMode = false; refreshTransportButtons(); };
    songButton.onClick = [this] { state.transport.songMode = true;  refreshTransportButtons(); };
    patButton.setConnectedEdges (juce::Button::ConnectedOnBottom);
    songButton.setConnectedEdges (juce::Button::ConnectedOnTop);

    playButton.setTooltip ("Play / Pause (Space)");
    stopButton.setTooltip ("Stop (Space)");
    recordButton.setTooltip ("Record (R) - coming soon");
    playButton.onClick = [this] { togglePlay(); };
    stopButton.onClick = [this] { stop(); };
    recordButton.setClickingTogglesState (true);

    for (auto* c : std::initializer_list<juce::Component*> { &patButton, &songButton, &playButton, &stopButton,
                                                            &recordButton, &tempo, &time, &masterVolume, &masterPitch,
                                                            &snap, &pattern, &patternAdd, &cpu })
        addAndMakeVisible (c);

    time.setTooltip ("Song position. Click to switch Bar:Step:Tick / Min:Sec");
    cpu.setTooltip ("CPU load");

    masterVolume.setRange (0.0, 1.0);
    masterVolume.setValue (state.transport.masterVolume.load(), juce::dontSendNotification);
    masterVolume.setDoubleClickReturnValue (true, 0.8);
    masterVolume.setTooltip ("Master volume");
    masterVolume.onValueChange = [this] { state.transport.masterVolume = (float) masterVolume.getValue(); };

    masterPitch.setRange (-1200.0, 1200.0, 1.0);
    masterPitch.setValue (0.0, juce::dontSendNotification);
    masterPitch.setDoubleClickReturnValue (true, 0.0);
    masterPitch.setTooltip ("Master pitch (cents)");

    // Global snap, same list and order as the spec
    int id = 1;
    snap.addItem ("Line", id++);
    snap.addItem ("Cell", id++);
    snap.addItem ("(none)", id++);
    snap.addSeparator();
    for (auto t : { "1/6 step", "1/4 step", "1/3 step", "1/2 step", "Step" }) snap.addItem (t, id++);
    snap.addSeparator();
    for (auto t : { "1/6 beat", "1/4 beat", "1/3 beat", "1/2 beat", "Beat" }) snap.addItem (t, id++);
    snap.addSeparator();
    snap.addItem ("Bar", id++);
    snap.setSelectedId (8, juce::dontSendNotification); // Step
    snap.setTooltip ("Global snap");

    pattern.addItem ("Pattern 1", 1);
    pattern.setSelectedId (1, juce::dontSendNotification);
    pattern.setTooltip ("Pattern selector");
    patternAdd.setTooltip ("Find next empty pattern (F4)");
    patternAdd.onClick = [this]
    {
        const int n = pattern.getNumItems() + 1;
        pattern.addItem ("Pattern " + juce::String (n), n);
        pattern.setSelectedId (n);
    };

    struct Def { const char* name; IconButton::Icon icon; const char* tip; };
    const Def defs[] = {
        { "Playlist",      IconButton::Icon::playlist,     "Playlist (F5)" },
        { "Piano roll",    IconButton::Icon::pianoRoll,    "Piano roll (F7)" },
        { "Channel rack",  IconButton::Icon::channelRack,  "Channel rack (F6)" },
        { "Mixer",         IconButton::Icon::mixer,        "Mixer (F9)" },
        { "Browser",       IconButton::Icon::browser,      "Browser (Alt+F8)" },
        { "Plugin picker", IconButton::Icon::pluginPicker, "Plugin picker (F8)" },
    };
    for (int i = 0; i < (int) std::size (defs); ++i)
    {
        auto* b = shortcuts.add (new IconButton (defs[i].name, defs[i].icon));
        b->setTooltip (defs[i].tip);
        b->onClick = [this, i] { if (onShowWindow) onShowWindow (i); };
        addAndMakeVisible (b);
    }

    refreshTransportButtons();
    startTimerHz (30);
}

void ToolbarPanel::togglePlay()
{
    state.transport.playing = ! state.transport.playing.load();
    refreshTransportButtons();
}

void ToolbarPanel::stop()
{
    state.transport.stop();
    refreshTransportButtons();
    time.repaint();
}

void ToolbarPanel::togglePatSong()
{
    state.transport.songMode = ! state.transport.songMode.load();
    refreshTransportButtons();
}

void ToolbarPanel::refreshTransportButtons()
{
    const bool song = state.transport.songMode;
    patButton.setToggleState (! song, juce::dontSendNotification);
    songButton.setToggleState (song, juce::dontSendNotification);
    playButton.setToggleState (state.transport.playing, juce::dontSendNotification);
}

void ToolbarPanel::timerCallback()
{
    time.repaint();
    tempo.repaint();
    if (playButton.getToggleState() != state.transport.playing.load())
        refreshTransportButtons();

    static int n = 0;
    if (++n % 10 == 0) cpu.repaint();
}

void ToolbarPanel::paint (juce::Graphics& g)
{
    g.fillAll (theme::bgRaised);
    g.setColour (theme::outline);
    g.drawHorizontalLine (getHeight() - 1, 0.0f, (float) getWidth());
}

void ToolbarPanel::resized()
{
    auto r = getLocalBounds().reduced (6, 5);
    const int h = r.getHeight();

    auto gap = [&r] (int px) { r.removeFromLeft (px); };

    auto patSong = r.removeFromLeft (46);
    patButton.setBounds (patSong.removeFromTop (h / 2));
    songButton.setBounds (patSong);
    gap (4);

    playButton.setBounds (r.removeFromLeft (h));   gap (2);
    stopButton.setBounds (r.removeFromLeft (h));   gap (2);
    recordButton.setBounds (r.removeFromLeft (h)); gap (10);

    tempo.setBounds (r.removeFromLeft (104)); gap (6);
    time.setBounds (r.removeFromLeft (108));  gap (10);

    masterVolume.setBounds (r.removeFromLeft (h)); gap (2);
    masterPitch.setBounds (r.removeFromLeft (h));  gap (10);

    snap.setBounds (r.removeFromLeft (110).withSizeKeepingCentre (110, 24)); gap (10);
    pattern.setBounds (r.removeFromLeft (130).withSizeKeepingCentre (130, 24)); gap (2);
    patternAdd.setBounds (r.removeFromLeft (24).withSizeKeepingCentre (24, 24)); gap (10);

    cpu.setBounds (r.removeFromLeft (72)); gap (10);

    // Window shortcuts aligned right
    for (int i = shortcuts.size(); --i >= 0;)
    {
        shortcuts[i]->setBounds (r.removeFromRight (h));
        r.removeFromRight (2);
    }
}
