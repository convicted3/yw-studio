#include "PlaylistView.h"
#include "LookAndFeel.h"

using namespace yw;

namespace
{
    constexpr int toolbarH = 22, topAreaH = 40, rulerH = 16, trackH = 56, numTracks = 100;
    constexpr int pickerW = 158, headerW = 100;

    struct Clip { int track; double start, length; int pattern; };

    juce::Colour clipColour (int pattern)
    {
        static const juce::uint32 cols[] = { 0xff5d7a6a, 0xff6a6f8f, 0xff8a6b55, 0xff4f7b86, 0xff7d5f78, 0xff7a7a52 };
        return juce::Colour (cols[(pattern - 1) % 6]);
    }
}

//==============================================================================
class PlaylistView::Picker : public juce::Component
{
public:
    explicit Picker (AppState& s) : state (s)
    {
        struct T { icons::Id icon; const char* hint; };
        const T t[] = { { icons::Id::keys, "Show patterns" }, { icons::Id::wave, "Show audio clips" }, { icons::Id::automation, "Show automation clips" } };
        for (int i = 0; i < 3; ++i)
        {
            auto* b = tabs.add (new ToolButton (t[i].icon, t[i].hint, ToolButton::Style::flat));
            b->setClickingTogglesState (true);
            b->setToggleState (i == 0, juce::dontSendNotification);
            b->setIconColour (i == 0 ? theme::textBright : theme::textDim);
            addAndMakeVisible (b);
        }
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (theme::pickerBg);
        auto r = getLocalBounds().withTrimmedTop (topAreaH);

        for (int i = 1; i <= state.numPatterns; ++i)
        {
            auto row = r.removeFromTop (26).reduced (4, 1);
            const bool cur = i == state.currentPattern;
            g.setColour (cur ? juce::Colour (0xff2a333a) : juce::Colour (0xff232b31));
            g.fillRect (row);
            if (cur)
            {
                juce::Path p;
                p.addTriangle ((float) row.getX() + 2.0f, row.getY() + 6.0f, (float) row.getX() + 2.0f, row.getBottom() - 6.0f, (float) row.getX() + 9.0f, (float) row.getCentreY());
                g.setColour (theme::accent);
                g.fillPath (p);
            }
            g.setColour (juce::Colour (0xffd0d3d5));
            g.fillEllipse ((float) row.getX() + 12.0f, row.getCentreY() - 1.5f, 3.0f, 3.0f);
            g.setColour (cur ? theme::textBright : theme::text);
            g.setFont (theme::font (17.0f));
            g.drawText ("Pattern " + juce::String (i), row.withTrimmedLeft (20), juce::Justification::centredLeft);
        }

        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawVerticalLine (getWidth() - 1, 0.0f, (float) getHeight());
    }

    void resized() override
    {
        auto top = getLocalBounds().removeFromTop (topAreaH).reduced (8, 8);
        const int w = top.getWidth() / 3;
        for (auto* t : tabs) t->setBounds (top.removeFromLeft (w).reduced (8, 0));
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        const int i = (e.y - topAreaH) / 26 + 1;
        if (e.y > topAreaH && i >= 1 && i <= state.numPatterns) { state.currentPattern = i; getParentComponent()->repaint(); }
    }

    AppState& state;
    juce::OwnedArray<ToolButton> tabs;
};

//==============================================================================
class PlaylistView::TrackList : public juce::Component
{
public:
    TrackList()
    {
        addButton.setIconScale (0.6f);
        addAndMakeVisible (addButton);
        for (int i = 0; i < numTracks; ++i) muted.push_back (false);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (theme::panelHeader);

        // top area: + and NOTE / CHAN / PAT focus switches
        auto top = getLocalBounds().removeFromTop (topAreaH);
        g.setColour (juce::Colour (0xff272e30));
        g.fillRect (top);
        const char* labels[] = { "NOTE", "CHAN", "PAT" };
        const icons::Id ic[] = { icons::Id::wave, icons::Id::automation, icons::Id::keys };
        auto sw = top.withTrimmedLeft (30).reduced (0, 2);
        const int w = sw.getWidth() / 3;
        for (int i = 0; i < 3; ++i)
        {
            auto cell = sw.removeFromLeft (w);
            const bool on = i == 2;
            icons::draw (g, ic[i], cell.removeFromTop (20).toFloat().reduced (3.0f), on ? theme::textBright : theme::textDim);
            g.setColour (on ? theme::textBright : theme::textDim);
            g.setFont (theme::font (10.0f, theme::Weight::semibold));
            g.drawText (labels[i], cell, juce::Justification::centred);
        }

        juce::Graphics::ScopedSaveState s (g);
        g.reduceClipRegion (getLocalBounds().withTrimmedTop (topAreaH));
        for (int i = 0; i < numTracks; ++i)
        {
            const int y = topAreaH + i * trackH - scrollY;
            if (y + trackH < topAreaH || y > getHeight()) continue;
            auto row = juce::Rectangle<int> (0, y, getWidth(), trackH);

            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2b3236), 0, (float) row.getY(), juce::Colour (0xff252b2f), 0, (float) row.getBottom(), false));
            g.fillRect (row.reduced (0, 1).withTrimmedRight (8));
            g.setColour (juce::Colour (0xff1a1f22));
            g.fillRect (row.getRight() - 8, row.getY(), 8, row.getHeight());
            g.setColour (juce::Colours::black.withAlpha (0.45f));
            g.drawHorizontalLine (row.getBottom() - 1, 0.0f, (float) getWidth());

            g.setColour (muted[(size_t) i] ? theme::textDim.withAlpha (0.5f) : theme::textDim);
            g.setFont (theme::font (16.0f));
            g.drawText ("Track " + juce::String (i + 1), row.reduced (5, 3).removeFromTop (18), juce::Justification::centredLeft);
            g.setFont (theme::font (13.0f));
            g.drawText ("...", row.reduced (22, 2).removeFromBottom (14), juce::Justification::bottomLeft);

            auto dot = juce::Rectangle<float> ((float) row.getRight() - 25.0f, (float) row.getBottom() - 19.0f, 13.0f, 13.0f);
            g.setColour (juce::Colour (0xff101315));
            g.fillEllipse (dot.expanded (1.0f));
            g.setColour (muted[(size_t) i] ? theme::textDim.withAlpha (0.4f) : theme::textBright);
            g.drawEllipse (dot, 1.4f);
            g.fillEllipse (dot.reduced (3.0f));
        }
    }

    void resized() override { addButton.setBounds (4, 10, 22, 22); }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.y < topAreaH) return;
        const int i = (e.y - topAreaH + scrollY) / trackH;
        if (i >= 0 && i < numTracks && e.x > getWidth() - 34) { muted[(size_t) i] = ! muted[(size_t) i]; repaint(); }
    }

    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override
    {
        if (onScroll) onScroll (w.deltaY);
    }

    juce::String getTooltipFor (int) { return {}; }

    int scrollY = 0;
    std::vector<bool> muted;
    ToolButton addButton { icons::Id::plus, "Add playlist track / arrangement", ToolButton::Style::flat };
    std::function<void (float)> onScroll;
};

//==============================================================================
class PlaylistView::Grid : public juce::Component, public juce::TooltipClient, private juce::Timer
{
public:
    explicit Grid (AppState& s) : state (s) { startTimerHz (30); }

    juce::String getTooltip() override { return "Playlist - click to place the current pattern, right-click to delete, drag to move"; }

    double barWidth = 48.0;
    int scrollX = 0, scrollY = 0;
    std::vector<Clip> clips;

    int barsVisible() const { return (int) std::ceil (getWidth() / barWidth) + 1; }

    void paint (juce::Graphics& g) override
    {
        const int w = getWidth(), h = getHeight();

        // ruler
        auto ruler = juce::Rectangle<int> (0, 0, w, rulerH);
        g.setColour (theme::ruler);
        g.fillRect (ruler);
        const int firstBar = (int) (scrollX / barWidth);
        g.setFont (theme::font (13.0f, theme::Weight::medium));
        for (int b = firstBar; b < firstBar + barsVisible(); ++b)
        {
            const float x = (float) (b * barWidth - scrollX);
            g.setColour (theme::text);
            g.drawText (juce::String (b + 1), juce::Rectangle<float> (x + 3.0f, 0.0f, 40.0f, (float) rulerH), juce::Justification::centredLeft);
            g.setColour (theme::textDim.withAlpha (0.5f));
            g.fillRect (x, rulerH - 5.0f, 1.0f, 5.0f);
        }

        // grid
        juce::Graphics::ScopedSaveState s (g);
        g.reduceClipRegion (0, rulerH, w, h - rulerH);

        for (int b = firstBar; b < firstBar + barsVisible(); ++b)
        {
            const float x = (float) (b * barWidth - scrollX);
            for (int beat = 0; beat < 4; ++beat)
            {
                g.setColour (beat % 2 == 0 ? theme::gridA : theme::gridB);
                g.fillRect (x + (float) (beat * barWidth / 4.0), (float) rulerH, (float) (barWidth / 4.0), (float) h);
            }
        }
        for (int b = firstBar; b < firstBar + barsVisible(); ++b)
        {
            const float x = (float) (b * barWidth - scrollX);
            g.setColour (theme::gridBeat);
            for (int beat = 1; beat < 4; ++beat) g.fillRect (x + (float) (beat * barWidth / 4.0), (float) rulerH, 1.0f, (float) h);
            g.setColour (juce::Colour (0xff070d10));
            g.fillRect (x, (float) rulerH, 1.0f, (float) h);
        }
        for (int t = 0; t <= numTracks; ++t)
        {
            const int y = rulerH + t * trackH - scrollY;
            if (y < rulerH || y > h) continue;
            g.setColour (juce::Colour (0xff070d10));
            g.fillRect (0, y - 1, w, 2);
        }

        // clips
        for (auto& c : clips)
        {
            auto r = clipBounds (c);
            if (! r.intersects (getLocalBounds().toFloat())) continue;
            auto col = clipColour (c.pattern);
            g.setColour (col.darker (0.35f));
            g.fillRoundedRectangle (r, 2.0f);
            auto head = r.removeFromTop (14.0f);
            g.setColour (col.brighter (0.15f));
            g.fillRoundedRectangle (head, 2.0f);
            g.setColour (juce::Colours::black.withAlpha (0.8f));
            g.setFont (theme::font (13.0f, theme::Weight::semibold));
            g.drawText ("Pattern " + juce::String (c.pattern), head.reduced (3.0f, 0.0f), juce::Justification::centredLeft, true);
            // note preview bars
            g.setColour (col.brighter (0.5f).withAlpha (0.8f));
            for (int i = 0; i < 4; ++i)
                g.fillRect (r.getX() + 3.0f + i * (r.getWidth() - 6.0f) / 4.0f, r.getBottom() - 10.0f - (float) ((i * 7) % 3) * 6.0f,
                            juce::jmax (2.0f, (r.getWidth() - 6.0f) / 8.0f), 3.0f);
            g.setColour (juce::Colours::black.withAlpha (0.6f));
            g.drawRoundedRectangle (clipBounds (c), 2.0f, 1.0f);
        }

        // playhead
        if (state.transport.songMode)
        {
            const double bars = state.transport.positionTicks.load() / (Transport::ticksPerStep * Transport::stepsPerBar);
            const float x = (float) (bars * barWidth - scrollX);
            g.setColour (juce::Colour (0xff8fd36a));
            g.fillRect (x, 0.0f, 1.5f, (float) h);
            juce::Path p;
            p.addTriangle (x - 5.0f, 0.0f, x + 6.5f, 0.0f, x + 0.75f, 8.0f);
            g.fillPath (p);
        }
    }

    juce::Rectangle<float> clipBounds (const Clip& c) const
    {
        return { (float) (c.start * barWidth - scrollX) + 1.0f, (float) (rulerH + c.track * trackH - scrollY) + 1.0f,
                 (float) (c.length * barWidth) - 2.0f, (float) trackH - 3.0f };
    }

    int clipAt (juce::Point<float> p) const
    {
        for (int i = (int) clips.size(); --i >= 0;)
            if (clipBounds (clips[(size_t) i]).contains (p)) return i;
        return -1;
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.y < rulerH)
        {
            const double bars = juce::jmax (0.0, (e.x + scrollX) / barWidth);
            state.transport.songMode = true;
            state.transport.positionTicks = std::floor (bars) * Transport::ticksPerStep * Transport::stepsPerBar;
            return;
        }

        const int hit = clipAt (e.position);
        if (e.mods.isPopupMenu())
        {
            if (hit >= 0) { clips.erase (clips.begin() + hit); repaint(); }
            return;
        }

        if (hit >= 0) { dragging = hit; dragOffset = (e.x + scrollX) / barWidth - clips[(size_t) hit].start; return; }

        const int track = (e.y - rulerH + scrollY) / trackH;
        if (track < 0 || track >= numTracks) return;
        const double bar = std::floor ((e.x + scrollX) / barWidth);
        clips.push_back ({ track, bar, 1.0, state.currentPattern });
        dragging = (int) clips.size() - 1;
        dragOffset = (e.x + scrollX) / barWidth - bar;
        repaint();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragging < 0 || e.mods.isPopupMenu()) return;
        auto& c = clips[(size_t) dragging];
        const double raw = (e.x + scrollX) / barWidth - dragOffset;
        c.start = juce::jmax (0.0, e.mods.isAltDown() ? raw : std::round (raw));
        c.track = juce::jlimit (0, numTracks - 1, (e.y - rulerH + scrollY) / trackH);
        repaint();
    }

    void mouseUp (const juce::MouseEvent&) override { dragging = -1; }

    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override
    {
        if (e.mods.isCtrlDown() || e.mods.isCommandDown())
        {
            barWidth = juce::jlimit (12.0, 400.0, barWidth * (w.deltaY > 0 ? 1.15 : 1.0 / 1.15));
            repaint();
            if (onZoom) onZoom();
        }
        else if (onScroll) onScroll (w.deltaY);
    }

    void timerCallback() override { if (state.transport.playing && state.transport.songMode) repaint(); }

    AppState& state;
    int dragging = -1;
    double dragOffset = 0.0;
    std::function<void (float)> onScroll;
    std::function<void()> onZoom;
};

//==============================================================================
PlaylistView::PlaylistView (AppState& s) : state (s)
{
    picker    = std::make_unique<Picker> (state);
    trackList = std::make_unique<TrackList>();
    grid      = std::make_unique<Grid> (state);

    struct T { icons::Id icon; const char* hint; };
    const T t[] = {
        { icons::Id::pencil, "Draw (P)" }, { icons::Id::brush, "Paint (B)" }, { icons::Id::erase, "Delete (D)" },
        { icons::Id::mute, "Mute (T)" }, { icons::Id::slip, "Slip (S)" }, { icons::Id::slice, "Slice (C)" },
        { icons::Id::select, "Select (E)" }, { icons::Id::zoom, "Zoom (Z)" }, { icons::Id::listen, "Playback (Y)" } };
    for (int i = 0; i < (int) std::size (t); ++i)
    {
        auto* b = tools.add (new ToolButton (t[i].icon, t[i].hint, ToolButton::Style::flat));
        b->setRadioGroupId (91);
        b->setClickingTogglesState (true);
        b->onClick = [this, i] { currentTool = i; };
        addAndMakeVisible (b);
    }
    tools[0]->setToggleState (true, juce::dontSendNotification);

    menuButton.onClick = [this]
    {
        juce::PopupMenu m;
        m.addItem (1, "Edit", false);
        m.addItem (2, "Tools", false);
        m.addItem (3, "View", false);
        m.addItem (4, "Snap", false);
        m.addItem (5, "Select", false);
        m.addItem (6, "Zoom", false);
        m.addItem (7, "Time markers", false);
        m.addSeparator();
        m.addItem (8, "Clear all clips");
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&menuButton), [this] (int r)
        { if (r == 8) { grid->clips.clear(); grid->repaint(); } });
    };
    closeButton.onClick = [this] { if (onClose) onClose(); };
    minButton.onClick   = [this] { if (onClose) onClose(); };

    for (auto* c : std::initializer_list<juce::Component*> { &menuButton, &snapButton, &closeButton, &minButton,
                                                            picker.get(), trackList.get(), grid.get(), &vScroll, &hScroll })
        addAndMakeVisible (c);

    auto scrollBy = [this] (float dy)
    {
        vScroll.setCurrentRangeStart (vScroll.getCurrentRangeStart() - dy * 120.0);
    };
    trackList->onScroll = scrollBy;
    grid->onScroll = scrollBy;
    grid->onZoom = [this] { resized(); };

    vScroll.addListener (this);
    hScroll.addListener (this);
    vScroll.setAutoHide (false);
    hScroll.setAutoHide (false);
}

PlaylistView::~PlaylistView()
{
    vScroll.removeListener (this);
    hScroll.removeListener (this);
}

void PlaylistView::scrollBarMoved (juce::ScrollBar* bar, double start)
{
    if (bar == &vScroll) { trackList->scrollY = grid->scrollY = (int) start; trackList->repaint(); }
    else                 grid->scrollX = (int) start;
    grid->repaint();
}

void PlaylistView::paint (juce::Graphics& g)
{
    g.fillAll (theme::panelHeader);

    // title breadcrumb
    auto bar = getLocalBounds().removeFromTop (toolbarH);
    auto title = bar.withX (tools.getLast()->getRight() + 16).withRight (minButton.getX() - 8);
    icons::draw (g, icons::Id::speaker, title.removeFromLeft (20).toFloat().reduced (2.0f), theme::text);
    title.removeFromLeft (6);
    g.setFont (theme::font (18.0f, theme::Weight::medium));
    const juce::String a = "Playlist - Arrangement", b = "Pattern " + juce::String (state.currentPattern);
    const int wa = (int) juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), a);
    g.setColour (theme::text);
    g.drawText (a, title, juce::Justification::centredLeft, true);
    auto arrowAt = [&] (int x)
    {
        juce::Path p;
        const float cy = (float) title.getCentreY();
        p.addTriangle ((float) x, cy - 4.0f, (float) x, cy + 4.0f, x + 5.0f, cy);
        g.setColour (theme::textDim);
        g.fillPath (p);
    };
    arrowAt (title.getX() + wa + 8);
    g.setColour (theme::text);
    g.drawText (b, title.withTrimmedLeft (wa + 20), juce::Justification::centredLeft, true);
    arrowAt (title.getX() + wa + 20 + (int) juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), b) + 8);
}

void PlaylistView::resized()
{
    auto r = getLocalBounds();
    auto bar = r.removeFromTop (toolbarH).reduced (2, 1);
    menuButton.setBounds (bar.removeFromLeft (20));
    snapButton.setBounds (bar.removeFromLeft (24));
    bar.removeFromLeft (8);
    for (auto* t : tools) t->setBounds (bar.removeFromLeft (24));
    closeButton.setBounds (bar.removeFromRight (20));
    minButton.setBounds (bar.removeFromRight (20));

    picker->setBounds (r.removeFromLeft (pickerW));
    auto bottom = r.removeFromBottom (12);
    vScroll.setBounds (r.removeFromRight (12));
    trackList->setBounds (r.removeFromLeft (headerW));
    hScroll.setBounds (bottom.withTrimmedLeft (headerW).withTrimmedRight (12));

    // grid starts below the area that holds the NOTE/CHAN/PAT switches, minus the ruler
    grid->setBounds (r.withTrimmedTop (topAreaH - rulerH));

    const double contentH = numTracks * trackH;
    const double viewH = juce::jmax (1, grid->getHeight() - rulerH);
    vScroll.setRangeLimits (0.0, contentH);
    vScroll.setCurrentRange (vScroll.getCurrentRangeStart(), viewH);

    const double contentW = 500 * grid->barWidth;
    hScroll.setRangeLimits (0.0, contentW);
    hScroll.setCurrentRange (hScroll.getCurrentRangeStart(), (double) grid->getWidth());
}
