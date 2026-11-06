#include "ChannelRackView.h"
#include "LookAndFeel.h"

using namespace yw;

namespace
{
    // Green mute LED
    class MuteLed : public juce::Component, public juce::TooltipClient
    {
    public:
        juce::String getTooltip() override { return on ? "Channel enabled - click to mute, right-click to solo" : "Channel muted"; }
        void paint (juce::Graphics& g) override
        {
            auto r = getLocalBounds().toFloat().withSizeKeepingCentre (11.0f, 11.0f);
            g.setColour (juce::Colour (0xff111518));
            g.fillEllipse (r);
            if (on)
            {
                g.setColour (theme::green.withAlpha (0.35f));
                g.fillEllipse (r.expanded (1.5f));
                g.setColour (theme::green);
                g.fillEllipse (r.reduced (2.0f));
            }
            g.setColour (juce::Colour (0xff070a0b));
            g.drawEllipse (r, 1.0f);
        }
        void mouseDown (const juce::MouseEvent&) override { on = ! on; repaint(); }
        bool on = true;
    };

    // Mixer track number box
    class TrackBox : public juce::Component, public juce::TooltipClient
    {
    public:
        juce::String getTooltip() override { return "Target mixer track: " + (track == 0 ? juce::String ("Master") : juce::String (track)); }
        void paint (juce::Graphics& g) override
        {
            auto r = getLocalBounds().toFloat().reduced (0.5f);
            g.setColour (juce::Colour (0xff2a3034));
            g.fillRoundedRectangle (r, 2.5f);
            g.setColour (theme::outline);
            g.drawRoundedRectangle (r, 2.5f, 1.0f);
            g.setColour (theme::text);
            g.setFont (theme::font (20.0f));
            g.drawText (track == 0 ? juce::String ("--") : juce::String (track), getLocalBounds().withTrimmedRight (9), juce::Justification::centredRight);
            g.setColour (theme::textDim);
            g.setFont (theme::font (9.0f));
            g.drawText (juce::String::fromUTF8 ("\xe2\x8c\x83"), getLocalBounds().removeFromRight (9), juce::Justification::centred);
        }
        void mouseDrag (const juce::MouseEvent& e) override
        {
            track = juce::jlimit (0, 125, start - e.getDistanceFromDragStartY() / 6);
            repaint();
        }
        void mouseDown (const juce::MouseEvent&) override { start = track; }
        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override
        {
            track = juce::jlimit (0, 125, track + (w.deltaY > 0 ? 1 : -1));
            repaint();
        }
        int track = 1, start = 1;
    };

    class NameButton : public juce::Component, public juce::TooltipClient
    {
    public:
        explicit NameButton (juce::String n) : name (std::move (n)) {}
        juce::String getTooltip() override { return name + " - click to open, right-click for options"; }
        void paint (juce::Graphics& g) override
        {
            auto r = getLocalBounds().toFloat().reduced (0.5f);
            const bool over = isMouseOver();
            g.setGradientFill (juce::ColourGradient (juce::Colour (over ? 0xff4a5357 : 0xff424a4e), 0, r.getY(),
                                                     juce::Colour (over ? 0xff353c40 : 0xff30373b), 0, r.getBottom(), false));
            g.fillRoundedRectangle (r, 3.0f);
            g.setColour (theme::outline);
            g.drawRoundedRectangle (r, 3.0f, 1.0f);
            g.setColour (theme::text);
            g.setFont (theme::font (16.0f));
            g.drawText (name, getLocalBounds().reduced (6, 0), juce::Justification::centred, true);
        }
        void mouseEnter (const juce::MouseEvent&) override { repaint(); }
        void mouseExit (const juce::MouseEvent&) override  { repaint(); }
        void mouseDown (const juce::MouseEvent& e) override
        {
            if (! e.mods.isPopupMenu()) return;
            juce::PopupMenu m;
            m.addItem (1, "Piano roll", false);
            m.addItem (2, "Graph editor", false);
            m.addSeparator();
            m.addItem (3, "Rename, color and icon...  F2", false);
            m.addItem (4, "Load sample...", false);
            m.addSeparator();
            m.addItem (5, "Insert", false);
            m.addItem (6, "Replace", false);
            m.addItem (7, "Clone", false);
            m.addItem (8, "Delete...", false);
            m.addSeparator();
            m.addItem (9, "Fill each 2 steps", true);
            m.addItem (10, "Fill each 4 steps", true);
            m.addItem (11, "Fill each 8 steps", true);
            m.addSeparator();
            m.addItem (12, "Rotate left   Shift+Ctrl+Left", true);
            m.addItem (13, "Rotate right  Shift+Ctrl+Right", true);
            m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [this] (int r) { if (onMenu) onMenu (r); });
        }
        juce::String name;
        std::function<void (int)> onMenu;
    };

    class Selector : public juce::Component, public juce::TooltipClient
    {
    public:
        juce::String getTooltip() override { return "Select channel"; }
        void paint (juce::Graphics& g) override
        {
            auto r = getLocalBounds().toFloat().withSizeKeepingCentre (8.0f, (float) getHeight() - 6.0f);
            g.setColour (selected ? theme::textBright : juce::Colour (0xff1c2124));
            g.fillRoundedRectangle (r, 2.5f);
            g.setColour (selected ? theme::textBright : juce::Colour (0xff6b7377));
            g.drawRoundedRectangle (r, 2.5f, 1.4f);
            if (selected) { g.setColour (juce::Colour (0xff1c2124)); g.fillRoundedRectangle (r.reduced (2.5f), 1.5f); }
        }
        void mouseDown (const juce::MouseEvent&) override { selected = ! selected; repaint(); }
        bool selected = true;
    };

    // 16-step sequencer strip
    class StepStrip : public juce::Component, public juce::TooltipClient
    {
    public:
        juce::String getTooltip() override { return "Step sequencer - left-click to set, right-click to clear, drag to paint"; }

        void paint (juce::Graphics& g) override
        {
            const int n = (int) steps.size();
            const float w = (float) getWidth() / n;
            for (int i = 0; i < n; ++i)
            {
                auto r = juce::Rectangle<float> (i * w, 0.0f, w, (float) getHeight()).reduced (1.5f, 2.0f);
                const bool groupB = (i / 4) % 2 == 1;
                juce::Colour base = steps[(size_t) i] ? theme::stepOn : (groupB ? theme::stepB : theme::stepA);
                g.setGradientFill (juce::ColourGradient (base.brighter (0.08f), 0, r.getY(), base.darker (0.18f), 0, r.getBottom(), false));
                g.fillRoundedRectangle (r, 2.0f);
                g.setColour (juce::Colours::black.withAlpha (0.55f));
                g.drawRoundedRectangle (r, 2.0f, 1.0f);
                if (! steps[(size_t) i])
                {
                    g.setColour (juce::Colours::white.withAlpha (0.06f));
                    g.fillRect (r.reduced (2.0f).removeFromTop (r.getHeight() * 0.35f));
                }
            }
        }

        int stepAt (float x) const { return juce::jlimit (0, (int) steps.size() - 1, (int) (x / ((float) getWidth() / (float) steps.size()))); }

        void mouseDown (const juce::MouseEvent& e) override
        {
            paintValue = ! e.mods.isPopupMenu() && ! steps[(size_t) stepAt (e.position.x)];
            if (e.mods.isPopupMenu()) paintValue = false;
            steps[(size_t) stepAt (e.position.x)] = paintValue;
            repaint();
        }
        void mouseDrag (const juce::MouseEvent& e) override
        {
            if (! getLocalBounds().contains (e.getPosition())) return;
            steps[(size_t) stepAt (e.position.x)] = paintValue;
            repaint();
        }

        std::array<bool, 16> steps {};
        bool paintValue = true;
    };
}

//==============================================================================
class ChannelRackView::ChannelRow : public juce::Component
{
public:
    explicit ChannelRow (const juce::String& name) : nameButton (name)
    {
        for (auto* c : std::initializer_list<juce::Component*> { &led, &pan, &vol, &track, &nameButton, &selector, &strip })
            addAndMakeVisible (c);

        vol.setDisplay ([] (float v) { return v <= 0.0001f ? juce::String ("-inf dB")
                                                         : juce::String (juce::Decibels::gainToDecibels (v / 0.78f) - 5.2f, 1) + " dB"; });
        nameButton.onMenu = [this] (int r)
        {
            auto& s = strip.steps;
            if (r >= 9 && r <= 11) { const int every = r == 9 ? 2 : r == 10 ? 4 : 8; for (size_t i = 0; i < s.size(); ++i) s[i] = i % (size_t) every == 0; }
            if (r == 12) std::rotate (s.begin(), s.begin() + 1, s.end());
            if (r == 13) std::rotate (s.rbegin(), s.rbegin() + 1, s.rend());
            strip.repaint();
        };
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (theme::rackRow);
        g.fillRect (getLocalBounds().withTrimmedLeft (strip.getX() - 6));
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (0, 2);
        r.removeFromLeft (6);
        led.setBounds (r.removeFromLeft (16));
        r.removeFromLeft (3);
        pan.setBounds (r.removeFromLeft (22).withSizeKeepingCentre (20, 20));
        vol.setBounds (r.removeFromLeft (22).withSizeKeepingCentre (20, 20));
        r.removeFromLeft (4);
        track.setBounds (r.removeFromLeft (32).reduced (0, 1));
        r.removeFromLeft (8);
        nameButton.setBounds (r.removeFromLeft (82).reduced (0, 1));
        r.removeFromLeft (6);
        selector.setBounds (r.removeFromLeft (12));
        r.removeFromLeft (10);
        strip.setBounds (r.removeFromLeft (juce::jmin (r.getWidth() - 8, 16 * 22)).reduced (0, 1));
    }

    MuteLed led;
    Knob pan { "Channel panning", 0.5f, true };
    Knob vol { "Channel volume", 0.78f };
    TrackBox track;
    NameButton nameButton;
    Selector selector;
    StepStrip strip;
};

//==============================================================================
ChannelRackView::ChannelRackView (AppState& s) : state (s)
{
    menuButton.onClick = [this]
    {
        juce::PopupMenu m;
        m.addItem (1, "Add one", true);
        m.addItem (2, "Select unused channels", false);
        m.addItem (3, "Clone selected  Alt+C", false);
        m.addItem (4, "Delete selected  Alt+Del", false);
        m.addSeparator();
        m.addItem (5, "Group selected  Alt+G", false);
        m.addItem (6, "Zip selected  Alt+Z", false);
        m.addSeparator();
        m.addItem (7, "Detached", false);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&menuButton), [this] (int r)
        { if (r == 1) addChannel ("Sampler"); });
    };

    filter.addItem ("All", 1);
    filter.addItem ("Unsorted", 2);
    filter.setSelectedId (1, juce::dontSendNotification);
    filter.setTooltip ("Channel display filter");
    lengthBox.setTooltip ("Pattern length (auto)");
    closeButton.onClick = [this] { if (onClose) onClose(); };
    stepsButton.setToggleState (false, juce::dontSendNotification);
    addButton.setIconScale (0.55f);
    addButton.onClick = [this]
    {
        juce::PopupMenu m;
        m.addItem (1, "Sampler");
        m.addSeparator();
        int id = 100;
        for (auto& d : state.knownPlugins.getTypes())
            if (d.isInstrument) m.addItem (id++, d.name, false);
        if (id == 100) m.addItem (2, "More plugins... (Options > Manage plugins)", false);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&addButton), [this] (int r)
        { if (r == 1) addChannel ("Sampler"); });
    };

    for (auto* c : std::initializer_list<juce::Component*> { &menuButton, &undoButton, &filter, &swing, &lengthBox,
                                                            &graphButton, &stepsButton, &addButton })
        addAndMakeVisible (c);

    addChannel ("Sampler");
}

ChannelRackView::~ChannelRackView() = default;

void ChannelRackView::addChannel (const juce::String& name)
{
    juce::String n = name;
    if (! rows.isEmpty()) n << " #" << (rows.size() + 1);
    addAndMakeVisible (rows.add (new ChannelRow (n)));
    resized();
}

void ChannelRackView::paint (juce::Graphics& g)
{
    g.fillAll (theme::panelHeader);

    auto r = getLocalBounds();
    auto header = r.removeFromTop (headerHeight);
    drawPanelHeader (g, header, {}, icons::Id::none);

    // separator dots
    g.setColour (theme::textDim.withAlpha (0.6f));
    const int dx = filter.getRight() + 12;
    for (int i = 0; i < 3; ++i) g.fillRect (dx, 7 + i * 4, 2, 2);

    // title
    auto title = header.withX (dx + 10).withRight (swing.getX() - 6);
    icons::draw (g, icons::Id::speaker, title.removeFromLeft (20).toFloat().reduced (2.0f), theme::text);
    g.setColour (theme::text);
    g.setFont (theme::font (18.0f, theme::Weight::medium));
    g.drawText ("Channel rack", title.withTrimmedLeft (4), juce::Justification::centredLeft, true);

    // pattern length box
    auto lb = lengthBox.getBounds().toFloat().reduced (0.5f);
    g.setColour (juce::Colour (0xff2a3034));
    g.fillRoundedRectangle (lb, 3.0f);
    g.setColour (theme::outline);
    g.drawRoundedRectangle (lb, 3.0f, 1.0f);
    g.setColour (theme::textDim);
    g.setFont (theme::font (14.0f));
    g.drawText (juce::String::fromUTF8 ("\xc2\xb7\xc2\xb7\xc2\xb7"), lb.toNearestInt().withTrimmedRight (8), juce::Justification::centredRight);

    // body
    auto body = r.reduced (6, 4);
    const int rowsH = rows.size() * rowHeight + 8;
    auto rack = body.removeFromTop (rowsH);
    g.setColour (theme::panelBody);
    g.fillRoundedRectangle (rack.toFloat(), 3.0f);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawRoundedRectangle (rack.toFloat(), 3.0f, 1.0f);
}

void ChannelRackView::resized()
{
    auto r = getLocalBounds();
    auto header = r.removeFromTop (headerHeight).reduced (2, 1);
    menuButton.setBounds (header.removeFromLeft (20));
    undoButton.setBounds (header.removeFromLeft (22));
    header.removeFromLeft (40);
    filter.setBounds (header.removeFromLeft (80).reduced (0, 1));
    header.removeFromRight (yw::captionButtonsWidth);
    stepsButton.setBounds (header.removeFromRight (24));
    graphButton.setBounds (header.removeFromRight (24));
    header.removeFromRight (6);
    lengthBox.setBounds (header.removeFromRight (36).reduced (0, 1));
    header.removeFromRight (6);
    swing.setBounds (header.removeFromRight (22));

    auto body = r.reduced (6, 4);
    auto rack = body.removeFromTop (rows.size() * rowHeight + 8).reduced (2, 4);
    for (auto* row : rows) row->setBounds (rack.removeFromTop (rowHeight));

    addButton.setBounds (body.removeFromTop (28).withSizeKeepingCentre (26, 24).withX (rows.isEmpty() ? 100 : rows[0]->getX() + 150 - 13));
}
