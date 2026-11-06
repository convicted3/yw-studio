#include "PlaylistView.h"
#include "LookAndFeel.h"

using namespace yw;

namespace
{
    constexpr int toolbarH = 22, topAreaH = 40, rulerH = 16, numTracks = 100, totalBars = 1000;
    constexpr int pickerW = 158, headerW = 100;

    // Grid colours (layered: background, step, beat and bar lines, track separators)
    const juce::Colour gridBg     { 0xff1e282e };
    const juce::Colour gridBgAlt  { 0xff1b252b };
    const juce::Colour stepLine   { 0xff141e24 };
    const juce::Colour beatLine   { 0xff0c161c };
    const juce::Colour barLine    { 0xff00040a };
    const juce::Colour rowEdge    { 0xff141e24 };
    const juce::Colour rowEdgeMid { 0xff020c12 };

    juce::Colour clipColour (int pattern)
    {
        static const juce::uint32 cols[] = { 0xff5d7a6a, 0xff6a6f8f, 0xff8a6b55, 0xff4f7b86, 0xff7d5f78, 0xff7a7a52 };
        return juce::Colour (cols[(pattern - 1) % 6]);
    }
}

//==============================================================================
float PlaylistView::Model::yOf (int track) const
{
    float y = 0.0f;
    for (int i = 0; i < track && i < (int) tracks.size(); ++i) y += tracks[(size_t) i].height;
    return y;
}

float PlaylistView::Model::totalHeight() const { return yOf ((int) tracks.size()); }

int PlaylistView::Model::trackAt (float contentY) const
{
    if (contentY < 0.0f) return -1;
    float y = 0.0f;
    for (int i = 0; i < (int) tracks.size(); ++i)
    {
        y += tracks[(size_t) i].height;
        if (contentY < y) return i;
    }
    return -1;
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
                p.addTriangle ((float) row.getX() + 2.0f, row.getY() + 6.0f, (float) row.getX() + 2.0f, row.getBottom() - 6.0f,
                               (float) row.getX() + 9.0f, (float) row.getCentreY());
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
class PlaylistView::TrackList : public juce::Component, public juce::TooltipClient
{
public:
    explicit TrackList (Model& m) : model (m)
    {
        addButton.setIconScale (0.6f);
        addAndMakeVisible (addButton);
    }

    enum class Zone { none, body, dots, mute, edge };

    struct Hit { int track = -1; Zone zone = Zone::none; };

    juce::Rectangle<float> rowBounds (int i) const
    {
        return { 0.0f, topAreaH + model.yOf (i) - model.scrollY, (float) getWidth(), model.tracks[(size_t) i].height };
    }

    static juce::Rectangle<float> dotsArea (juce::Rectangle<float> row)  { return { row.getX() + 16.0f, row.getBottom() - 17.0f, 26.0f, 13.0f }; }
    static juce::Rectangle<float> muteArea (juce::Rectangle<float> row, bool collapsed)
    {
        return collapsed ? juce::Rectangle<float> (row.getRight() - 24.0f, row.getY(), 16.0f, row.getHeight())
                         : juce::Rectangle<float> (row.getRight() - 28.0f, row.getBottom() - 22.0f, 19.0f, 19.0f);
    }

    Hit hitTestAt (juce::Point<float> p) const
    {
        Hit h;
        if (p.y < topAreaH) return h;
        h.track = model.trackAt (p.y - topAreaH + model.scrollY);
        if (h.track < 0) return h;

        const auto row = rowBounds (h.track);
        // the resize handle straddles the line between two tracks
        if (p.y < row.getY() + 3.0f && h.track > 0) { h.track -= 1; h.zone = Zone::edge; return h; }
        const auto& t = model.tracks[(size_t) h.track];
        if (p.y > row.getBottom() - 5.0f) h.zone = Zone::edge;
        else if (muteArea (row, t.collapsed).contains (p)) h.zone = Zone::mute;
        else if (! t.collapsed && dotsArea (row).expanded (2.0f).contains (p)) h.zone = Zone::dots;
        else h.zone = Zone::body;
        return h;
    }

    juce::String getTooltip() override
    {
        const auto h = hitTestAt (getMouseXYRelative().toFloat());
        if (h.track < 0) return {};
        const auto& t = model.tracks[(size_t) h.track];
        switch (h.zone)
        {
            case Zone::edge: return "Drag to resize the track";
            case Zone::mute: return t.muted ? "Track muted - click to unmute" : "Track enabled - click to mute";
            case Zone::dots: return "Double-click to collapse the track";
            default:         return t.collapsed ? "Click to restore the track" : "Track " + juce::String (h.track + 1);
        }
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (theme::panelHeader);

        // top area: + and NOTE / CHAN / PAT focus switches
        auto top = getLocalBounds().removeFromTop (topAreaH);
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

        for (int i = 0; i < (int) model.tracks.size(); ++i)
        {
            const auto row = rowBounds (i);
            if (row.getBottom() < topAreaH || row.getY() > getHeight()) continue;
            const auto& t = model.tracks[(size_t) i];
            const bool hoverRow = i == hover.track;

            auto body = row.withTrimmedRight (8.0f).withTrimmedBottom (2.0f);
            g.setGradientFill (juce::ColourGradient (juce::Colour (hoverRow ? 0xff2f373b : 0xff2b3236), 0, body.getY(),
                                                     juce::Colour (hoverRow ? 0xff292f33 : 0xff252b2f), 0, body.getBottom(), false));
            g.fillRect (body);
            g.setColour (juce::Colour (0xff1a1f22));
            g.fillRect (row.withLeft (row.getRight() - 8.0f));
            g.setColour (juce::Colours::black.withAlpha (0.55f));
            g.fillRect (row.withTop (row.getBottom() - 2.0f));

            const auto nameColour = t.muted ? theme::textDim.withAlpha (0.45f) : theme::textDim;

            if (t.collapsed || row.getHeight() < 26.0f)
            {
                g.setColour (nameColour);
                g.setFont (theme::font (juce::jmin (15.0f, row.getHeight() - 2.0f)));
                g.drawText ("Track " + juce::String (i + 1), body.reduced (5.0f, 0.0f).toNearestInt(), juce::Justification::centredLeft, true);
            }
            else
            {
                g.setColour (nameColour);
                g.setFont (theme::font (16.0f));
                g.drawText ("Track " + juce::String (i + 1), body.reduced (5.0f, 3.0f).removeFromTop (18.0f).toNearestInt(), juce::Justification::centredLeft);

                // "..." collapse handle, highlighted on hover
                const auto dots = dotsArea (row);
                const bool hot = hoverRow && hover.zone == Zone::dots;
                if (hot)
                {
                    g.setColour (juce::Colours::white.withAlpha (0.1f));
                    g.fillRoundedRectangle (dots, 3.0f);
                }
                g.setColour (hot ? theme::textBright : theme::textDim.withAlpha (0.8f));
                for (int d = 0; d < 3; ++d)
                    g.fillEllipse (dots.getX() + 6.0f + d * 5.5f, dots.getCentreY() + 1.0f, 2.5f, 2.5f);
            }

            // mute dot
            const auto mute = muteArea (row, t.collapsed);
            const float sz = t.collapsed ? juce::jmin (10.0f, row.getHeight() - 6.0f) : 13.0f;
            auto dot = mute.withSizeKeepingCentre (sz, sz);
            g.setColour (juce::Colour (0xff101315));
            g.fillEllipse (dot.expanded (1.0f));
            g.setColour (t.muted ? theme::textDim.withAlpha (0.4f) : theme::textBright);
            g.drawEllipse (dot, 1.4f);
            g.fillEllipse (dot.reduced (sz * 0.23f));
        }
    }

    void resized() override { addButton.setBounds (4, 10, 22, 22); }

    void mouseMove (const juce::MouseEvent& e) override
    {
        const auto h = hitTestAt (e.position);
        setMouseCursor (h.zone == Zone::edge ? juce::MouseCursor::UpDownResizeCursor : juce::MouseCursor::NormalCursor);
        if (h.track != hover.track || h.zone != hover.zone) { hover = h; repaint(); }
    }

    void mouseExit (const juce::MouseEvent&) override { hover = {}; repaint(); }

    void mouseDown (const juce::MouseEvent& e) override
    {
        const auto h = hitTestAt (e.position);
        resizing = -1;
        if (h.track < 0) return;
        auto& t = model.tracks[(size_t) h.track];

        if (h.zone == Zone::edge)
        {
            resizing = h.track;
            resizeStart = t.height;
            return;
        }
        if (h.zone == Zone::mute) { t.muted = ! t.muted; repaint(); if (onChanged) onChanged(); return; }

        if (t.collapsed && ! e.mods.isPopupMenu())
        {
            t.collapsed = false;
            t.target = t.restore;   // animated back to its previous height
            if (onChanged) onChanged();
        }
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (resizing < 0) return;
        auto& t = model.tracks[(size_t) resizing];
        const float h = juce::jlimit (Model::minHeight, Model::maxHeight, resizeStart + (float) e.getDistanceFromDragStartY());
        t.height = t.target = t.restore = h;
        t.collapsed = false;
        if (onChanged) onChanged();
    }

    void mouseUp (const juce::MouseEvent&) override { resizing = -1; }

    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
        const auto h = hitTestAt (e.position);
        if (h.track < 0) return;
        auto& t = model.tracks[(size_t) h.track];
        if (h.zone == Zone::dots && ! t.collapsed)
        {
            t.restore = t.target;
            t.target = Model::collapsedHeight;   // animated
            t.collapsed = true;
            if (onChanged) onChanged();
        }
        else if (h.zone == Zone::edge)
        {
            t.target = t.restore = Model::defaultHeight; // double-click the edge: default size
            t.collapsed = false;
            if (onChanged) onChanged();
        }
    }

    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override
    {
        if (onScroll) onScroll (w.deltaY);
    }

    Model& model;
    Hit hover;
    int resizing = -1;
    float resizeStart = 0.0f;
    ToolButton addButton { icons::Id::plus, "Add playlist track / arrangement", ToolButton::Style::flat };
    std::function<void (float)> onScroll;
    std::function<void()> onChanged;
};

//==============================================================================
class PlaylistView::Grid : public juce::Component, public juce::TooltipClient, private juce::Timer
{
public:
    Grid (AppState& s, Model& m) : state (s), model (m) { startTimerHz (30); }

    juce::String getTooltip() override
    {
        if (getMouseXYRelative().y < rulerH) return "Timeline - mouse wheel to zoom, click to set the song position";
        return "Playlist - click to place the current pattern, right-click to delete, drag to move, Ctrl+wheel to zoom";
    }

    float rowTop (int track) const { return (float) rulerH + model.yOf (track) - model.scrollY; }

    void paint (juce::Graphics& g) override
    {
        const int w = getWidth(), h = getHeight();
        const double bw = model.barWidth;
        const double beatW = bw / 4.0, stepW = bw / 16.0;
        const int firstBar = juce::jmax (0, (int) (model.scrollX / bw));
        const int lastBar  = (int) ((model.scrollX + w) / bw) + 1;
        auto xOf = [&] (double bars) { return (float) (bars * bw - model.scrollX); };

        // --- body background ------------------------------------------------------
        g.setColour (gridBg);
        g.fillRect (0, rulerH, w, h - rulerH);
        if (bw < 90.0)
            for (int b = firstBar; b <= lastBar; ++b)
                if (b % 2 == 1)
                {
                    g.setColour (gridBgAlt);
                    g.fillRect (xOf (b), (float) rulerH, (float) bw, (float) (h - rulerH));
                }

        // --- vertical lines: steps, beats, bars --------------------------------
        const int barEvery = bw < 14.0 ? 4 : 1;
        for (int b = firstBar; b <= lastBar; ++b)
        {
            if (stepW >= 7.0)
            {
                g.setColour (stepLine);
                for (int s = 1; s < 16; ++s) if (s % 4 != 0) g.fillRect (xOf (b + s / 16.0), (float) rulerH, 1.0f, (float) h);
            }
            if (beatW >= 6.0)
            {
                g.setColour (beatLine);
                for (int q = 1; q < 4; ++q) g.fillRect (xOf (b + q / 4.0), (float) rulerH, 1.0f, (float) h);
            }
            if (b % barEvery == 0)
            {
                g.setColour (barLine);
                g.fillRect (xOf (b), (float) rulerH, 1.0f, (float) h);
            }
        }

        // --- track separators ----------------------------------------------------
        {
            juce::Graphics::ScopedSaveState s (g);
            g.reduceClipRegion (0, rulerH, w, h - rulerH);
            for (int t = 0; t < (int) model.tracks.size(); ++t)
            {
                const float bottom = rowTop (t) + model.tracks[(size_t) t].height;
                if (bottom < rulerH - 3 || bottom > h + 3) continue;
                g.setColour (rowEdge);    g.fillRect (0.0f, bottom - 3.0f, (float) w, 1.0f);
                g.setColour (rowEdgeMid); g.fillRect (0.0f, bottom - 2.0f, (float) w, 1.0f);
                g.setColour (rowEdge);    g.fillRect (0.0f, bottom - 1.0f, (float) w, 1.0f);
            }

            // --- clips --------------------------------------------------------------
            for (auto& c : model.clips) drawClip (g, c);
        }

        // --- ruler ---------------------------------------------------------------
        g.setColour (theme::ruler);
        g.fillRect (0, 0, w, rulerH);
        const int labelEvery = bw < 22.0 ? 4 : bw < 34.0 ? 2 : 1;
        for (int b = firstBar; b <= lastBar; ++b)
        {
            const float x = xOf (b);
            if (b % labelEvery == 0)
            {
                g.setColour (theme::text);
                g.setFont (theme::font (13.0f, theme::Weight::medium));
                g.drawText (juce::String (b + 1), juce::Rectangle<float> (x + 3.0f, 0.0f, 44.0f, (float) rulerH), juce::Justification::centredLeft);
                g.setColour (theme::textDim.withAlpha (0.6f));
                g.fillRect (x, rulerH - 6.0f, 1.0f, 6.0f);
            }
            if (beatW >= 34.0)
                for (int q = 1; q < 4; ++q)
                {
                    const float bx = xOf (b + q / 4.0);
                    g.setColour (theme::textDim.withAlpha (0.35f));
                    g.fillRect (bx, rulerH - 3.0f, 1.0f, 3.0f);
                    if (beatW >= 60.0)
                    {
                        g.setColour (theme::textDim);
                        g.setFont (theme::font (11.0f));
                        g.drawText (juce::String (b + 1) + "." + juce::String (q + 1), juce::Rectangle<float> (bx + 2.0f, 1.0f, 40.0f, (float) rulerH - 2.0f), juce::Justification::centredLeft);
                    }
                }
        }

        // --- playhead --------------------------------------------------------------
        if (state.transport.songMode)
        {
            const double bars = state.transport.positionTicks.load() / (Transport::ticksPerStep * Transport::stepsPerBar);
            const float x = xOf (bars);
            g.setColour (juce::Colour (0xff8fd36a));
            g.fillRect (x, 0.0f, 1.5f, (float) h);
            juce::Path p;
            p.addTriangle (x - 5.0f, 0.0f, x + 6.5f, 0.0f, x + 0.75f, 8.0f);
            g.fillPath (p);
        }
    }

    juce::Rectangle<float> clipBounds (const Clip& c) const
    {
        const auto& t = model.tracks[(size_t) c.track];
        return { (float) (c.start * model.barWidth - model.scrollX) + 1.0f, rowTop (c.track) + 1.0f,
                 (float) (c.length * model.barWidth) - 2.0f, juce::jmax (6.0f, t.height - 4.0f) };
    }

    void drawClip (juce::Graphics& g, const Clip& c)
    {
        auto r = clipBounds (c);
        if (! r.intersects (getLocalBounds().toFloat())) return;
        const auto col = clipColour (c.pattern);
        const float headH = juce::jmin (15.0f, r.getHeight());

        g.setColour (col.darker (0.35f));
        g.fillRoundedRectangle (r, 2.0f);
        auto head = r.withHeight (headH);
        g.setColour (col.brighter (0.15f));
        g.fillRoundedRectangle (head, 2.0f);

        if (r.getWidth() > 24.0f)
        {
            g.setColour (juce::Colours::black.withAlpha (0.8f));
            g.setFont (theme::font (juce::jmin (13.0f, headH - 1.0f), theme::Weight::semibold));
            g.drawText ("Pattern " + juce::String (c.pattern), head.reduced (3.0f, 0.0f), juce::Justification::centredLeft, true);
        }

        // note preview, scales with the clip
        auto body = r.withTrimmedTop (headH).reduced (3.0f, 3.0f);
        if (body.getHeight() > 6.0f)
        {
            g.setColour (col.brighter (0.55f).withAlpha (0.85f));
            const float noteH = juce::jlimit (2.0f, 6.0f, body.getHeight() / 8.0f);
            for (int i = 0; i < 4; ++i)
            {
                const float y = body.getY() + body.getHeight() * (0.25f + 0.25f * (float) ((i * 7) % 3));
                g.fillRect (body.getX() + i * body.getWidth() / 4.0f, y, juce::jmax (2.0f, body.getWidth() / 8.0f), noteH);
            }
        }

        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawRoundedRectangle (r, 2.0f, 1.0f);
    }

    int clipAt (juce::Point<float> p) const
    {
        for (int i = (int) model.clips.size(); --i >= 0;)
            if (clipBounds (model.clips[(size_t) i]).contains (p)) return i;
        return -1;
    }

    int trackAtY (float y) const { return model.trackAt (y - rulerH + model.scrollY); }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.y < rulerH)
        {
            const double bars = juce::jmax (0.0, (e.x + model.scrollX) / model.barWidth);
            state.transport.songMode = true;
            state.transport.positionTicks = std::floor (bars * 4.0) / 4.0 * Transport::ticksPerStep * Transport::stepsPerBar;
            repaint();
            return;
        }

        const int hit = clipAt (e.position);
        if (e.mods.isPopupMenu())
        {
            if (hit >= 0) { model.clips.erase (model.clips.begin() + hit); repaint(); }
            return;
        }

        if (hit >= 0) { dragging = hit; dragOffset = (e.x + model.scrollX) / model.barWidth - model.clips[(size_t) hit].start; return; }

        const int track = trackAtY (e.position.y);
        if (track < 0) return;
        const double bar = std::floor ((e.x + model.scrollX) / model.barWidth);
        model.clips.push_back ({ track, bar, 1.0, state.currentPattern });
        dragging = (int) model.clips.size() - 1;
        dragOffset = (e.x + model.scrollX) / model.barWidth - bar;
        repaint();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragging < 0 || e.mods.isPopupMenu()) return;
        auto& c = model.clips[(size_t) dragging];
        const double raw = (e.x + model.scrollX) / model.barWidth - dragOffset;
        c.start = juce::jmax (0.0, e.mods.isAltDown() ? raw : std::round (raw));
        const int t = trackAtY (e.position.y);
        if (t >= 0) c.track = t;
        repaint();
    }

    void mouseUp (const juce::MouseEvent&) override { dragging = -1; }

    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override
    {
        const int steps = w.deltaY > 0 ? 1 : (w.deltaY < 0 ? -1 : 0);
        if (steps == 0) return;
        if (e.y < rulerH || e.mods.isCtrlDown() || e.mods.isCommandDown())
        {
            if (onZoom) onZoom ((double) e.x, steps);
        }
        else if (e.mods.isShiftDown())
        {
            if (onScrollX) onScrollX (w.deltaY);
        }
        else if (onScroll) onScroll (w.deltaY);
    }

    void timerCallback() override { if (state.transport.playing && state.transport.songMode) repaint(); }

    AppState& state;
    Model& model;
    int dragging = -1;
    double dragOffset = 0.0;
    std::function<void (float)> onScroll, onScrollX;
    std::function<void (double, int)> onZoom;
};


//==============================================================================
// Horizontal scroll bar above the timeline, like the classic playlist:
// drag the thumb to scroll, drag its ends to zoom, arrows scroll by a bar, wheel scrolls.
class PlaylistView::TimelineBar : public juce::Component, public juce::TooltipClient
{
public:
    std::function<void (double& start, double& length, double& total)> getRange; // in bars
    std::function<void (double startBars)> onScroll;
    std::function<void (double startBars, double lengthBars)> onZoom;

    static constexpr int arrowW = 14, minThumb = 18;

    juce::String getTooltip() override
    {
        const auto x = getMouseXYRelative().x;
        if (x < arrowW || x > getWidth() - arrowW) return "Scroll";
        return "Drag to scroll - drag the ends to zoom";
    }

    juce::Rectangle<float> track() const { return getLocalBounds().toFloat().reduced ((float) arrowW, 2.0f); }

    juce::Rectangle<float> thumb() const
    {
        double s = 0, l = 1, t = 1;
        if (getRange) getRange (s, l, t);
        auto tr = track();
        const float x0 = tr.getX() + (float) (s / t) * tr.getWidth();
        const float w  = juce::jmax ((float) minThumb, (float) (l / t) * tr.getWidth());
        return { juce::jmin (x0, tr.getRight() - w), tr.getY(), w, tr.getHeight() };
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff20272b));
        g.fillRect (r);

        auto tr = track();
        g.setColour (juce::Colour (0xff2b3337));
        g.fillRoundedRectangle (tr, 2.0f);

        auto th = thumb();
        const bool hot = isMouseOverOrDragging();
        g.setGradientFill (juce::ColourGradient (juce::Colour (hot ? 0xff4a555b : 0xff414b50), 0, th.getY(),
                                                 juce::Colour (hot ? 0xff3b454a : 0xff353e43), 0, th.getBottom(), false));
        g.fillRoundedRectangle (th, 2.0f);
        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.drawRoundedRectangle (th, 2.0f, 1.0f);
        // grips at both ends (zoom handles)
        g.setColour (theme::textDim.withAlpha (hot ? 0.8f : 0.45f));
        for (float gx : { th.getX() + 4.0f, th.getRight() - 6.0f })
            g.fillRect (gx, th.getY() + 4.0f, 2.0f, th.getHeight() - 8.0f);

        auto arrow = [&] (float cx, bool left)
        {
            juce::Path p;
            const float cy = r.getCentreY();
            p.startNewSubPath (cx + (left ? 2.0f : -2.0f), cy - 4.5f);
            p.lineTo (cx + (left ? -2.0f : 2.0f), cy);
            p.lineTo (cx + (left ? 2.0f : -2.0f), cy + 4.5f);
            g.setColour (theme::text);
            g.strokePath (p, juce::PathStrokeType (1.4f));
        };
        arrow ((float) arrowW * 0.5f, true);
        arrow ((float) getWidth() - arrowW * 0.5f, false);
    }

    enum class Drag { none, move, left, right };

    void mouseDown (const juce::MouseEvent& e) override
    {
        double s = 0, l = 1, t = 1;
        if (getRange) getRange (s, l, t);
        startS = s; startL = l; total = t;
        drag = Drag::none;

        if (e.x < arrowW)               { if (onScroll) onScroll (juce::jmax (0.0, s - 1.0)); return; }
        if (e.x > getWidth() - arrowW)  { if (onScroll) onScroll (s + 1.0); return; }

        auto th = thumb();
        if (std::abs (e.position.x - th.getX()) < 6.0f)           drag = Drag::left;
        else if (std::abs (e.position.x - th.getRight()) < 6.0f)  drag = Drag::right;
        else if (th.contains (e.position))                        drag = Drag::move;
        else
        {
            // click on the track: jump so the thumb centres on the mouse
            const double centre = (e.position.x - track().getX()) / track().getWidth() * t;
            if (onScroll) onScroll (juce::jmax (0.0, centre - l * 0.5));
            drag = Drag::move;
            if (getRange) getRange (startS, startL, total);
        }
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        const double barsPerPixel = total / juce::jmax (1.0f, track().getWidth());
        const double d = e.getDistanceFromDragStartX() * barsPerPixel;
        switch (drag)
        {
            case Drag::move:  if (onScroll) onScroll (juce::jmax (0.0, startS + d)); break;
            case Drag::left:
            {
                const double ns = juce::jlimit (0.0, startS + startL - 0.25, startS + d);
                if (onZoom) onZoom (ns, startS + startL - ns);
                break;
            }
            case Drag::right:
                if (onZoom) onZoom (startS, juce::jmax (0.25, startL + d));
                break;
            case Drag::none: break;
        }
    }

    void mouseMove (const juce::MouseEvent& e) override
    {
        auto th = thumb();
        const bool edge = std::abs (e.position.x - th.getX()) < 6.0f || std::abs (e.position.x - th.getRight()) < 6.0f;
        setMouseCursor (edge ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::NormalCursor);
        repaint();
    }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }
    void mouseUp (const juce::MouseEvent&) override { drag = Drag::none; repaint(); }

    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override
    {
        double s = 0, l = 1, t = 1;
        if (getRange) getRange (s, l, t);
        if (onScroll) onScroll (juce::jmax (0.0, s - (w.deltaY + w.deltaX) * l * 0.5));
    }

private:
    Drag drag = Drag::none;
    double startS = 0, startL = 1, total = 1;
};

//==============================================================================
PlaylistView::PlaylistView (AppState& s) : state (s)
{
    model.tracks.resize (numTracks);

    picker    = std::make_unique<Picker> (state);
    trackList = std::make_unique<TrackList> (model);
    grid      = std::make_unique<Grid> (state, model);
    timeline  = std::make_unique<TimelineBar>();

    timeline->getRange = [this] (double& start, double& length, double& total)
    {
        start  = model.scrollX / model.barWidth;
        length = juce::jmax (1, grid->getWidth()) / model.barWidth;
        total  = juce::jmax (songLengthBars(), start + length);
    };
    timeline->onScroll = [this] (double bars) { scrollToX (bars * model.barWidth); };
    timeline->onZoom = [this] (double startBars, double lengthBars)
    {
        const double bw = juce::jlimit (Model::minBarWidth, Model::maxBarWidth, juce::jmax (1, grid->getWidth()) / lengthBars);
        model.barWidth = model.targetBarWidth = bw;
        scrollToX (startBars * bw);
    };

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
        juce::PopupMenu m, zoom;
        zoom.addItem (20, "Zoom in   PgUp");
        zoom.addItem (21, "Zoom out  PgDn");
        zoom.addItem (22, "Default zoom");
        m.addSubMenu ("Zoom", zoom);
        m.addSeparator();
        m.addItem (10, "Restore all collapsed tracks");
        m.addItem (11, "Reset all track sizes");
        m.addSeparator();
        m.addItem (8, "Clear all clips");
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&menuButton), [this] (int r)
        {
            if (r == 8) { model.clips.clear(); grid->repaint(); }
            if (r == 10) for (auto& tr : model.tracks) if (tr.collapsed) { tr.collapsed = false; tr.target = tr.restore; }
            if (r == 11) for (auto& tr : model.tracks) { tr.collapsed = false; tr.target = tr.restore = Model::defaultHeight; }
            if (r == 20) zoomAt (grid->getWidth() * 0.5, 2);
            if (r == 21) zoomAt (grid->getWidth() * 0.5, -2);
            if (r == 22) { zoomAnchorBar = (model.scrollX + grid->getWidth() * 0.5) / model.barWidth; model.zoomAnchorX = grid->getWidth() * 0.5; model.targetBarWidth = 48.0; }
            if (! isTimerRunning()) startTimerHz (60);
        });
    };
    closeButton.onClick = [this] { if (onClose) onClose(); };
    minButton.onClick   = [this] { if (onClose) onClose(); };

    for (auto* c : std::initializer_list<juce::Component*> { &menuButton, &snapButton,
                                                            picker.get(), trackList.get(), grid.get(), &vScroll, timeline.get() })
        addAndMakeVisible (c);

    trackList->onScroll  = [this] (float dy) { scrollVertically (dy); };
    trackList->onChanged = [this] { if (! isTimerRunning()) startTimerHz (60); updateScrollBars(); grid->repaint(); trackList->repaint(); };
    grid->onScroll  = [this] (float dy) { scrollVertically (dy); };
    grid->onScrollX = [this] (float dx) { scrollToX (model.scrollX - dx * 300.0); };
    grid->onZoom    = [this] (double x, int steps) { zoomAt (x, steps); };

    vScroll.addListener (this);
    vScroll.setAutoHide (false);
}

double PlaylistView::songLengthBars() const
{
    double end = 0.0;
    for (auto& c : model.clips) end = juce::jmax (end, c.start + c.length);
    return juce::jmax (64.0, end + 16.0);
}

void PlaylistView::scrollToX (double x)
{
    model.scrollX = juce::jlimit (0.0, totalBars * model.barWidth, x);
    timeline->repaint();
    grid->repaint();
}

PlaylistView::~PlaylistView()
{
    vScroll.removeListener (this);
}

void PlaylistView::zoomAt (double viewX, int wheelSteps)
{
    // keep the bar under the mouse fixed while the zoom animates
    model.zoomAnchorX = viewX;
    zoomAnchorBar = (model.scrollX + viewX) / model.barWidth;
    model.targetBarWidth = juce::jlimit (Model::minBarWidth, Model::maxBarWidth,
                                         model.targetBarWidth * std::pow (1.22, (double) wheelSteps));
    if (! isTimerRunning()) startTimerHz (60);
}

void PlaylistView::scrollVertically (float deltaY)
{
    vScroll.setCurrentRangeStart (vScroll.getCurrentRangeStart() - deltaY * 140.0);
}

void PlaylistView::timerCallback()
{
    bool active = false;

    // smooth zoom
    if (std::abs (model.barWidth - model.targetBarWidth) > 0.01)
    {
        model.barWidth += (model.targetBarWidth - model.barWidth) * 0.25;
        if (std::abs (model.barWidth - model.targetBarWidth) < 0.02) model.barWidth = model.targetBarWidth;
        model.scrollX = juce::jmax (0.0, zoomAnchorBar * model.barWidth - model.zoomAnchorX);
        active = true;
    }

    // smooth track resize (collapse / restore)
    for (auto& t : model.tracks)
        if (std::abs (t.height - t.target) > 0.05f)
        {
            t.height += (t.target - t.height) * 0.3f;
            if (std::abs (t.height - t.target) < 0.3f) t.height = t.target;
            active = true;
        }

    if (active)
    {
        updateScrollBars();
        grid->repaint();
        trackList->repaint();
    }
    else stopTimer();
}

void PlaylistView::updateScrollBars()
{
    const juce::ScopedValueSetter<bool> svs (updatingScrollBars, true);

    const double viewH = juce::jmax (1, grid->getHeight() - rulerH);
    vScroll.setRangeLimits (0.0, juce::jmax ((double) model.totalHeight(), viewH));
    vScroll.setCurrentRange (model.scrollY, viewH);
    model.scrollY = (float) vScroll.getCurrentRangeStart();

    timeline->repaint();
}

void PlaylistView::scrollBarMoved (juce::ScrollBar* bar, double start)
{
    if (updatingScrollBars) return;
    if (bar == &vScroll) model.scrollY = (float) start;
    trackList->repaint();
    grid->repaint();
}

void PlaylistView::paint (juce::Graphics& g)
{
    g.fillAll (theme::panelHeader);

    auto bar = getLocalBounds().removeFromTop (toolbarH);
    auto title = bar.withX (tools.getLast()->getRight() + 16).withRight (getWidth() - yw::captionButtonsWidth - 8);
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
    bar.removeFromRight (yw::captionButtonsWidth);

    picker->setBounds (r.removeFromLeft (pickerW));
    vScroll.setBounds (r.removeFromRight (12).withTrimmedTop (topAreaH));
    trackList->setBounds (r.removeFromLeft (headerW));
    timeline->setBounds (r.getX(), r.getY() + 2, r.getWidth() + 12, topAreaH - rulerH - 4);

    // the grid's ruler sits at the bottom of the header area so rows line up with the track headers
    grid->setBounds (r.withTrimmedTop (topAreaH - rulerH));
    updateScrollBars();
}
