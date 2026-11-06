#include "MixerView.h"
#include "LookAndFeel.h"

using namespace yw;

namespace
{
    constexpr int stripWidth = 44;
    constexpr int numInserts = 125;

    class Fader : public juce::Component, public juce::TooltipClient
    {
    public:
        explicit Fader (bool master) : isMaster (master) {}
        juce::String getTooltip() override
        {
            const float db = value <= 0.0001f ? -100.0f : juce::Decibels::gainToDecibels (std::pow (value / 0.8f, 2.0f));
            return juce::String ("Track volume - ") + (db <= -99.0f ? juce::String ("-inf") : juce::String (db, 1)) + " dB";
        }
        void paint (juce::Graphics& g) override
        {
            auto r = getLocalBounds().toFloat();
            const float cx = r.getCentreX();
            const float top = 8.0f, bottom = r.getBottom() - 8.0f;
            const float y = bottom - value * (bottom - top);

            g.setColour (juce::Colour (0xff0e1214));
            g.fillRoundedRectangle (cx - 2.0f, top, 4.0f, bottom - top, 2.0f);
            if (isMaster)
            {
                g.setColour (theme::textBright.withAlpha (0.85f));
                g.fillRect (cx - 0.75f, y, 1.5f, bottom - y);
            }

            juce::Rectangle<float> cap (cx - 8.0f, y - 11.0f, 16.0f, 22.0f);
            g.setGradientFill (juce::ColourGradient (juce::Colour (isMaster ? 0xffe8ebec : 0xff5e676b), 0, cap.getY(),
                                                     juce::Colour (isMaster ? 0xffa9afb2 : 0xff383f43), 0, cap.getBottom(), false));
            g.fillRoundedRectangle (cap, 2.0f);
            g.setColour (juce::Colours::black.withAlpha (0.6f));
            g.drawRoundedRectangle (cap, 2.0f, 1.0f);
            g.setColour (juce::Colours::black.withAlpha (0.55f));
            g.fillRect (cap.getX() + 2.0f, cap.getCentreY() - 0.75f, cap.getWidth() - 4.0f, 1.5f);
        }
        void mouseDown (const juce::MouseEvent&) override { start = value; }
        void mouseDrag (const juce::MouseEvent& e) override
        {
            const float sens = e.mods.isShiftDown() ? 2000.0f : (float) juce::jmax (40, getHeight() - 16);
            value = juce::jlimit (0.0f, 1.0f, start - (float) e.getDistanceFromDragStartY() / sens);
            repaint();
        }
        void mouseDoubleClick (const juce::MouseEvent&) override { value = 0.8f; repaint(); }
        float value = 0.8f, start = 0.8f;
        bool isMaster;
    };

    class SmallDot : public juce::Component, public juce::TooltipClient
    {
    public:
        explicit SmallDot (juce::String hint, bool ring) : tip (std::move (hint)), withRing (ring) {}
        juce::String getTooltip() override { return tip; }
        void paint (juce::Graphics& g) override
        {
            auto r = getLocalBounds().toFloat().withSizeKeepingCentre (9.0f, 9.0f);
            if (withRing)
            {
                g.setColour (juce::Colour (0xff111518)); g.fillEllipse (r.expanded (2.0f));
                g.setColour (on ? theme::textBright : theme::textDim.withAlpha (0.5f));
                g.drawEllipse (r.expanded (1.0f), 1.2f);
                g.fillEllipse (r.reduced (1.5f));
            }
            else
            {
                g.setColour (on ? theme::textBright : theme::textDim.withAlpha (0.4f));
                g.fillEllipse (r.reduced (2.0f));
            }
        }
        void mouseDown (const juce::MouseEvent&) override { on = ! on; repaint(); }
        juce::String tip;
        bool withRing, on = true;
    };

    class IconGlyph : public juce::Component, public juce::TooltipClient
    {
    public:
        IconGlyph (icons::Id i, juce::String hint) : icon (i), tip (std::move (hint)) {}
        juce::String getTooltip() override { return tip; }
        void paint (juce::Graphics& g) override { icons::draw (g, icon, getLocalBounds().toFloat().reduced (2.0f), theme::textDim); }
        icons::Id icon;
        juce::String tip;
    };
}

//==============================================================================
class MixerView::Strip : public juce::Component
{
public:
    enum class Kind { scale, master, insert };

    Strip (Kind k, int index) : kind (k), number (index), fader (k == Kind::master),
        enable ("Mixer track enabled - click to mute", true),
        recDot ("Route to selected track", false),
        panIcon (icons::Id::slip, "Swap left / right"),
        stereoIcon (icons::Id::slip, "Stereo separation"),
        armIcon (icons::Id::mic, "Arm disk recording"),
        delayIcon (icons::Id::reset, "Track delay compensation")
    {
        if (kind == Kind::scale) return;
        for (auto* c : std::initializer_list<juce::Component*> { &enable, &pan, &stereoIcon, &sep, &fader, &armIcon, &delayIcon, &recDot })
            addAndMakeVisible (c);
        pan.setStyle (true);
    }

    juce::String name() const { return kind == Kind::master ? juce::String ("Master") : "Insert " + juce::String (number); }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds();
        g.setColour (kind == Kind::master ? juce::Colour (0xff22282c) : theme::mixerBg);
        g.fillRect (r);
        if (selected) { g.setColour (juce::Colours::white.withAlpha (0.035f)); g.fillRect (r); }

        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.drawVerticalLine (r.getRight() - 1, 0.0f, (float) getHeight());

        // number header
        auto head = r.removeFromTop (20);
        g.setColour (juce::Colour (0xff272e31));
        g.fillRect (head);
        g.setColour (kind == Kind::master ? theme::textBright : theme::textDim);
        g.setFont (theme::font (15.0f, kind == Kind::master ? theme::Weight::semibold : theme::Weight::regular));
        g.drawText (kind == Kind::scale ? juce::String ("C") : kind == Kind::master ? juce::String ("M") : juce::String (number),
                    head, juce::Justification::centred);

        auto meterArea = juce::Rectangle<int> (4, 26, getWidth() - 8, meterHeight);

        if (kind == Kind::scale)
        {
            g.setColour (theme::meterBg);
            g.fillRoundedRectangle (meterArea.toFloat().withTrimmedBottom (-200), 2.0f);
            g.setFont (theme::font (12.0f, theme::Weight::medium));
            const char* marks[] = { "3", "0", "3", "6", "9", "12", "15", "18", "21", "24", "27", "30", "33", "36" };
            const int step = juce::jmax (14, (getHeight() - 140) / 14);
            for (int i = 0; i < 14; ++i)
            {
                g.setColour (i < 2 ? theme::textBright : i > 9 ? juce::Colour (0xffd8a75a) : theme::text);
                g.drawText (marks[i], 0, 34 + i * step, getWidth(), 12, juce::Justification::centred);
            }
            g.setColour (theme::textDim);
            g.fillRect (6, 34 + 9 * step + 6, getWidth() - 12, 1);
            return;
        }

        // meter well with vertical name
        g.setColour (theme::meterBg);
        g.fillRoundedRectangle (meterArea.toFloat(), 2.0f);
        auto meters = meterArea.removeFromRight (12).reduced (2, 2);
        g.setColour (juce::Colour (0xff0a0d0f));
        g.fillRect (meters.removeFromLeft (4));
        meters.removeFromLeft (1);
        g.fillRect (meters.removeFromLeft (4));

        {
            juce::Graphics::ScopedSaveState s (g);
            g.addTransform (juce::AffineTransform::rotation (-juce::MathConstants<float>::halfPi)
                                .translated ((float) meterArea.getX() + 4.0f, (float) meterArea.getBottom() - 4.0f));
            g.setColour (kind == Kind::master ? theme::textBright : theme::textDim);
            g.setFont (theme::font (15.0f));
            g.drawText (name(), 0, 0, meterArea.getHeight() - 8, meterArea.getWidth() - 4, juce::Justification::centredLeft);
        }

        if (kind == Kind::master)
        {
            g.setColour (theme::textBright);
            g.drawRoundedRectangle (juce::Rectangle<float> (3.0f, 25.0f, getWidth() - 6.0f, (float) meterHeight + 2.0f), 2.0f, 1.5f);
        }

        // send arrow at the bottom (route to selected)
        juce::Path p;
        const float cx = getWidth() * 0.5f, by = getHeight() - 14.0f;
        p.addTriangle (cx - 4.0f, by + 3.0f, cx + 4.0f, by + 3.0f, cx, by - 3.0f);
        g.setColour (theme::textDim.withAlpha (0.4f));
        g.strokePath (p, juce::PathStrokeType (1.0f));
    }

    void resized() override
    {
        if (kind == Kind::scale) return;
        int y = 26 + meterHeight + 4;
        const int w = getWidth();
        enable.setBounds (0, y, w, 14);         y += 16;
        pan.setBounds ((w - 28) / 2, y, 28, 28); y += 32;
        stereoIcon.setBounds ((w - 16) / 2, y, 16, 14); y += 16;
        sep.setBounds ((w - 18) / 2, y, 18, 18);  y += 20;
        const int faderBottom = getHeight() - 70;
        fader.setBounds (0, y, w, juce::jmax (60, faderBottom - y));
        y = faderBottom + 6;
        armIcon.setBounds ((w - 14) / 2, y, 14, 14);   y += 18;
        delayIcon.setBounds ((w - 14) / 2, y, 14, 14); y += 18;
        recDot.setBounds (0, y, w, 10);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (onSelect) onSelect();
        if (e.mods.isPopupMenu() && kind != Kind::scale)
        {
            juce::PopupMenu m;
            m.addItem (1, "Rename, color and icon...  F2", false);
            m.addItem (2, "Assign to new audio track(s)", false);
            m.addSeparator();
            m.addItem (3, "Reset selected track(s) to default", true);
            m.addSeparator();
            m.addItem (4, "Solo  S", false);
            m.addItem (5, "Alt solo  Alt+S", false);
            m.showMenuAsync (juce::PopupMenu::Options(), [this] (int r)
            { if (r == 3) { fader.value = 0.8f; pan.setValue (0.5f); fader.repaint(); } });
        }
    }

    Kind kind;
    int number;
    bool selected = false;
    int meterHeight = 130;
    std::function<void()> onSelect;

    Fader fader;
    SmallDot enable, recDot;
    Knob pan { "Track panning", 0.5f, true };
    Knob sep { "Stereo separation", 0.5f, true };
    IconGlyph panIcon, stereoIcon, armIcon, delayIcon;
};

//==============================================================================
MixerView::MixerView (AppState& s) : state (s)
{
    menuButton.onClick = [this]
    {
        juce::PopupMenu m;
        m.addItem (1, "Disk recording", false);
        m.addItem (2, "Plugin delay compensation", false);
        m.addItem (3, "Link all parameters...", false);
        m.addSeparator();
        m.addItem (4, "View: Waveforms  Alt+W", false);
        m.addItem (5, "View: Names at top", false);
        m.addSeparator();
        m.addItem (6, "Detached", false);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&menuButton), [] (int) {});
    };
    addAndMakeVisible (menuButton);

    struct I { icons::Id icon; const char* hint; };
    const I hi[] = { { icons::Id::slip, "Extra track properties" }, { icons::Id::up, "Track inspector" },
                     { icons::Id::waitInput, "Track routing view" }, { icons::Id::project, "Mixer presets" } };
    for (auto& h : hi) addAndMakeVisible (headerIcons.add (new ToolButton (h.icon, h.hint, ToolButton::Style::flat)));

    for (auto t : { "Compact", "Compact 2", "Wide", "Extra large", "Extra wide" }) layout.addItem (t, layout.getNumItems() + 1);
    layout.setSelectedId (3, juce::dontSendNotification);
    layout.setTooltip ("Mixer layout preset");
    layout.setColour (juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);
    layout.setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (layout);
    closeButton.onClick = [this] { if (onClose) onClose(); };
    addAndMakeVisible (closeButton);

    strips.add (new Strip (Strip::Kind::scale, -1));
    strips.add (new Strip (Strip::Kind::master, 0));
    for (int i = 1; i <= numInserts; ++i) strips.add (new Strip (Strip::Kind::insert, i));
    for (int i = 0; i < strips.size(); ++i)
    {
        stripHolder.addAndMakeVisible (strips[i]);
        strips[i]->onSelect = [this, i]
        {
            if (strips[i]->kind == Strip::Kind::scale) return;
            for (auto* st : strips) { st->selected = false; st->repaint(); }
            strips[i]->selected = true;
            selected = i;
        };
    }
    strips[1]->selected = true;

    viewport.setViewedComponent (&stripHolder, false);
    viewport.setScrollBarsShown (false, true);
    viewport.setScrollBarThickness (10);
    addAndMakeVisible (viewport);
}

MixerView::~MixerView() = default;

void MixerView::paint (juce::Graphics& g)
{
    g.fillAll (theme::mixerBg);
    drawPanelHeader (g, getLocalBounds().removeFromTop (24), {}, icons::Id::none);
}

void MixerView::resized()
{
    auto r = getLocalBounds();
    auto header = r.removeFromTop (24).reduced (2, 2);
    menuButton.setBounds (header.removeFromLeft (20));
    for (auto* b : headerIcons) b->setBounds (header.removeFromLeft (22));
    header.removeFromLeft (4);
    layout.setBounds (header.removeFromLeft (90));
    closeButton.setBounds (header.removeFromRight (22));

    viewport.setBounds (r);
    const int h = juce::jmax (300, r.getHeight() - viewport.getScrollBarThickness());
    const int meterH = juce::jlimit (90, 200, h / 3);

    int x = 0;
    for (int i = 0; i < strips.size(); ++i)
    {
        const int w = i == 0 ? 36 : stripWidth;
        strips[i]->meterHeight = meterH;
        strips[i]->setBounds (x, 0, w, h);
        x += w + (i == 1 ? 8 : 0);  // small gap after master
    }
    stripHolder.setSize (x, h);
}
