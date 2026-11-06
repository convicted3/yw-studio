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
// Track inspector (right side): input, 10 effect slots, equalizer, send and output.
class MixerView::Inspector : public juce::Component, public juce::TooltipClient
{
public:
    static constexpr int width = 224;

    Inspector (AppState& s, std::vector<TrackFx>& f) : state (s), fx (f)
    {
        for (int i = 0; i < 10; ++i)
        {
            auto* k = mixKnobs.add (new Knob ("Mix level", 1.0f));
            k->setStyle (true);
            k->onChange = [this, i] (float v) { fx[(size_t) track].mix[(size_t) i] = v; };
            addAndMakeVisible (k);
        }
        const char* eqNames[] = { "Low band frequency", "Mid band frequency", "High band frequency",
                                  "Low band width", "Mid band width", "High band width" };
        for (auto* n : eqNames) addAndMakeVisible (eqKnobs.add (new Knob (n, 0.5f, true)));
    }

    void setTrack (int index, const juce::String& name)
    {
        track = juce::jlimit (0, (int) fx.size() - 1, index);
        trackName = name;
        for (int i = 0; i < 10; ++i) mixKnobs[i]->setValue (fx[(size_t) track].mix[(size_t) i], false);
        repaint();
    }

    juce::String getTooltip() override
    {
        auto p = getMouseXYRelative();
        if (inputRow.contains (p))  return "Audio input source for this track";
        if (sendRow.contains (p))   return "Track send target";
        if (outputRow.contains (p)) return "Audio output target for this track";
        if (eqGraph.contains (p))   return "Equalizer";
        for (int i = 0; i < 10; ++i)
            if (slotRows[(size_t) i].contains (p))
            {
                auto& n = fx[(size_t) track].names[(size_t) i];
                if (p.x > slotRows[(size_t) i].getRight() - 14)
                    return "Slot " + juce::String (i + 1) + (fx[(size_t) track].enabled[(size_t) i] ? " enabled - click to bypass" : " bypassed - click to enable");
                return n.isEmpty() ? "Slot " + juce::String (i + 1) + " - click to select an effect plugin"
                                   : n + " - click for slot options";
            }
        return {};
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xff20272b));
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawVerticalLine (0, 0.0f, (float) getHeight());

        // header
        auto header = getLocalBounds().removeFromTop (20);
        g.setColour (theme::panelHeader);
        g.fillRect (header);
        g.setColour (theme::text);
        g.setFont (theme::font (15.0f));
        g.drawText ("Mixer - " + trackName, header.reduced (8, 0), juce::Justification::centredLeft, true);

        auto& t = fx[(size_t) track];

        // input selector
        drawSelector (g, inputRow, "(none)", icons::Id::wave, true);

        // slots
        for (int i = 0; i < 10; ++i)
        {
            auto row = slotRows[(size_t) i];
            g.setColour (i % 2 == 0 ? juce::Colour (0xff262d31) : juce::Colour (0xff242b2f));
            g.fillRect (row);
            g.setColour (juce::Colours::black.withAlpha (0.45f));
            g.drawHorizontalLine (row.getBottom() - 1, (float) row.getX(), (float) row.getRight());

            auto r = row;
            juce::Path arrow;
            const float ay = (float) r.getCentreY();
            arrow.addTriangle ((float) r.getX() + 7.0f, ay - 4.0f, (float) r.getX() + 7.0f, ay + 4.0f, (float) r.getX() + 11.5f, ay);
            g.setColour (theme::textDim);
            g.fillPath (arrow);

            const auto& name = t.names[(size_t) i];
            g.setFont (theme::font (17.0f));
            g.setColour (name.isEmpty() ? juce::Colour (0xff5d676c) : (t.enabled[(size_t) i] ? theme::textBright : theme::textDim));
            g.drawText (name.isEmpty() ? "Slot " + juce::String (i + 1) : name,
                        r.withTrimmedLeft (18).withTrimmedRight (50), juce::Justification::centredLeft, true);

            // enable LED
            auto led = juce::Rectangle<float> ((float) r.getRight() - 12.0f, ay - 3.5f, 7.0f, 7.0f);
            g.setColour (t.enabled[(size_t) i] ? theme::textBright : juce::Colour (0xff3a4247));
            g.fillEllipse (led);
        }

        // equalizer
        g.setColour (juce::Colour (0xff1a2023));
        g.fillRect (eqGraph);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawRect (eqGraph, 1);
        g.setColour (juce::Colours::white.withAlpha (0.05f));
        for (int i = 1; i < 4; ++i) g.drawVerticalLine (eqGraph.getX() + i * eqGraph.getWidth() / 4, (float) eqGraph.getY(), (float) eqGraph.getBottom());
        g.setColour (theme::textBright.withAlpha (0.85f));
        g.fillRect (eqGraph.getX() + 2, eqGraph.getCentreY() + 8, eqGraph.getWidth() - 4, 2);

        for (int i = 0; i < 3; ++i)
        {
            auto f = eqFaders[(size_t) i];
            g.setColour (juce::Colour (0xff0e1214));
            g.fillRect (f.getCentreX() - 1, f.getY(), 3, f.getHeight());
            const int y = f.getY() + (int) ((1.0f - eqGains[(size_t) i]) * (float) (f.getHeight() - 10));
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff5e676b), 0, (float) y, juce::Colour (0xff383f43), 0, (float) y + 10.0f, false));
            g.fillRoundedRectangle (juce::Rectangle<float> ((float) f.getX(), (float) y, (float) f.getWidth(), 10.0f), 1.5f);
            g.setColour (juce::Colours::black.withAlpha (0.5f));
            g.fillRect (f.getX() + 1, y + 4, f.getWidth() - 2, 1);
        }

        g.setColour (theme::text);
        g.setFont (theme::font (17.0f));
        g.drawText ("Equalizer", eqLabel, juce::Justification::centredLeft);
        icons::draw (g, icons::Id::waitInput, eqIconA.toFloat(), theme::textDim);
        icons::draw (g, icons::Id::slip, eqIconB.toFloat(), theme::textDim);

        // send + output
        drawSelector (g, sendRow, "(none)", icons::Id::none, false);
        juce::String out = "Out 1 - Out 2";
        drawSelector (g, outputRow, out, icons::Id::download, true);
    }

    void resized() override
    {
        auto r = getLocalBounds().withTrimmedLeft (1).withTrimmedTop (20);
        r.removeFromTop (4);
        inputRow = r.removeFromTop (26).reduced (4, 2);
        r.removeFromTop (4);
        for (int i = 0; i < 10; ++i)
        {
            slotRows[(size_t) i] = r.removeFromTop (27).reduced (2, 0);
            auto row = slotRows[(size_t) i];
            mixKnobs[i]->setBounds (row.getRight() - 40, row.getY() + 2, 23, 23);
        }
        r.removeFromTop (10);

        auto eq = r.removeFromTop (100).reduced (4, 0);
        auto faders = eq.removeFromRight (58);
        eqGraph = eq.withTrimmedRight (6);
        for (int i = 0; i < 3; ++i) eqFaders[(size_t) i] = faders.removeFromLeft (19).reduced (3, 2);

        r.removeFromTop (12);
        auto eqRow = r.removeFromTop (44).reduced (4, 0);
        auto knobArea = eqRow.removeFromRight (72);
        eqLabel = eqRow.withTrimmedBottom (16);
        auto icons1 = knobArea.removeFromLeft (14);
        eqIconA = icons1.removeFromTop (22).reduced (1, 5);
        eqIconB = icons1.reduced (1, 5);
        for (int i = 0; i < 6; ++i)
            eqKnobs[i]->setBounds (knobArea.getX() + (i % 3) * 20, knobArea.getY() + (i / 3) * 22, 19, 19);

        r.removeFromTop (10);
        sendRow = r.removeFromTop (28).reduced (2, 2);
        r.removeFromTop (4);
        outputRow = r.removeFromTop (28).reduced (2, 2);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        const auto p = e.getPosition();
        for (int i = 0; i < 3; ++i)
            if (eqFaders[(size_t) i].expanded (3).contains (p)) { draggingFader = i; mouseDrag (e); return; }

        for (int i = 0; i < 10; ++i)
            if (slotRows[(size_t) i].contains (p))
            {
                if (p.x > slotRows[(size_t) i].getRight() - 14)
                {
                    auto& en = fx[(size_t) track].enabled[(size_t) i];
                    en = ! en;
                    repaint();
                }
                else showSlotMenu (i);
                return;
            }

        if (outputRow.contains (p)) showOutputMenu();
        else if (inputRow.contains (p)) showInputMenu();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (draggingFader < 0) return;
        auto f = eqFaders[(size_t) draggingFader];
        eqGains[(size_t) draggingFader] = juce::jlimit (0.0f, 1.0f, 1.0f - (float) (e.y - f.getY() - 5) / (float) juce::jmax (1, f.getHeight() - 10));
        repaint();
    }

    void mouseUp (const juce::MouseEvent&) override { draggingFader = -1; }
    void mouseDoubleClick (const juce::MouseEvent&) override { if (draggingFader >= 0) { eqGains[(size_t) draggingFader] = 0.5f; repaint(); } }

private:
    void drawSelector (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& text, icons::Id icon, bool framed)
    {
        auto box = r;
        auto iconArea = box.removeFromLeft (22);
        if (icon != icons::Id::none)
            icons::draw (g, icon, iconArea.toFloat().reduced (3.0f), theme::textDim);
        else
        {
            g.setColour (theme::textDim.withAlpha (0.5f));
            g.drawEllipse (iconArea.toFloat().reduced (5.0f), 1.2f);
        }
        if (framed)
        {
            g.setColour (juce::Colour (0xff171c1f));
            g.fillRoundedRectangle (box.toFloat(), 2.0f);
            g.setColour (juce::Colours::black.withAlpha (0.6f));
            g.drawRoundedRectangle (box.toFloat(), 2.0f, 1.0f);
        }
        g.setColour (theme::text);
        g.setFont (theme::font (16.0f));
        g.drawText (text, box.reduced (6, 0).withTrimmedRight (10), juce::Justification::centredLeft, true);
        juce::Path a;
        const float ax = (float) box.getRight() - 9.0f, ay = (float) box.getCentreY();
        a.addTriangle (ax - 2.0f, ay - 4.0f, ax - 2.0f, ay + 4.0f, ax + 2.5f, ay);
        g.setColour (theme::textDim);
        g.fillPath (a);
    }

    void showSlotMenu (int slot)
    {
        juce::PopupMenu m;
        m.addSectionHeader ("Slot " + juce::String (slot + 1));
        m.addItem (1, "(none)");
        m.addSeparator();

        juce::Array<juce::PluginDescription> effects;
        for (auto& d : state.knownPlugins.getTypes())
            if (! d.isInstrument) effects.add (d);

        if (effects.isEmpty())
            m.addItem (2, "No effect plugins yet - Options > Manage plugins", false);

        // group by vendor like the plugin database
        std::map<juce::String, juce::PopupMenu> byVendor;
        for (int i = 0; i < effects.size(); ++i)
            byVendor[effects[i].manufacturerName.isEmpty() ? juce::String ("Other") : effects[i].manufacturerName]
                .addItem (100 + i, effects[i].name + "  (" + effects[i].pluginFormatName + ")");
        for (auto& [vendor, sub] : byVendor) m.addSubMenu (vendor, sub);

        m.addSeparator();
        const bool has = fx[(size_t) track].names[(size_t) slot].isNotEmpty();
        m.addItem (3, "Move up", has && slot > 0);
        m.addItem (4, "Move down", has && slot < 9);

        m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (slotRows[(size_t) slot])),
                         [this, slot, effects] (int r)
        {
            auto& t = fx[(size_t) track];
            if (r == 1) t.names[(size_t) slot] = {};
            else if (r == 3) { std::swap (t.names[(size_t) slot], t.names[(size_t) slot - 1]); std::swap (t.mix[(size_t) slot], t.mix[(size_t) slot - 1]); std::swap (t.enabled[(size_t) slot], t.enabled[(size_t) slot - 1]); }
            else if (r == 4) { std::swap (t.names[(size_t) slot], t.names[(size_t) slot + 1]); std::swap (t.mix[(size_t) slot], t.mix[(size_t) slot + 1]); std::swap (t.enabled[(size_t) slot], t.enabled[(size_t) slot + 1]); }
            else if (r >= 100 && r - 100 < effects.size()) { t.names[(size_t) slot] = effects[r - 100].name; t.enabled[(size_t) slot] = true; }
            setTrack (track, trackName);
        });
    }

    juce::StringArray channelPairs (bool inputs)
    {
        juce::StringArray pairs;
        if (auto* dev = state.deviceManager.getCurrentAudioDevice())
        {
            auto names = inputs ? dev->getInputChannelNames() : dev->getOutputChannelNames();
            for (int i = 0; i + 1 < names.size(); i += 2)
                pairs.add (inputs ? "In " + juce::String (i + 1) + " - In " + juce::String (i + 2)
                                  : "Out " + juce::String (i + 1) + " - Out " + juce::String (i + 2));
        }
        return pairs;
    }

    void showOutputMenu()
    {
        juce::PopupMenu m;
        auto pairs = channelPairs (false);
        if (pairs.isEmpty()) m.addItem (1, "Out 1 - Out 2", true, true);
        for (int i = 0; i < pairs.size(); ++i) m.addItem (10 + i, pairs[i], true, i == 0);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (outputRow)));
    }

    void showInputMenu()
    {
        juce::PopupMenu m;
        m.addItem (1, "(none)", true, true);
        auto pairs = channelPairs (true);
        for (int i = 0; i < pairs.size(); ++i) m.addItem (10 + i, pairs[i], false);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (inputRow)));
    }

    AppState& state;
    std::vector<TrackFx>& fx;
    int track = 0;
    juce::String trackName { "Master" };
    juce::OwnedArray<Knob> mixKnobs, eqKnobs;
    std::array<juce::Rectangle<int>, 10> slotRows;
    std::array<juce::Rectangle<int>, 3> eqFaders;
    std::array<float, 3> eqGains { 0.5f, 0.5f, 0.5f };
    juce::Rectangle<int> inputRow, eqGraph, eqLabel, eqIconA, eqIconB, sendRow, outputRow;
    int draggingFader = -1;
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

    strips.add (new Strip (Strip::Kind::scale, -1));
    strips.add (new Strip (Strip::Kind::master, 0));
    for (int i = 1; i <= numInserts; ++i) strips.add (new Strip (Strip::Kind::insert, i));
    for (int i = 0; i < strips.size(); ++i)
    {
        stripHolder.addAndMakeVisible (strips[i]);
        strips[i]->onSelect = [this, i] { selectStrip (i); };
    }

    fx.resize ((size_t) numInserts + 1); // master + inserts
    inspector = std::make_unique<Inspector> (state, fx);
    addAndMakeVisible (*inspector);
    selectStrip (1);

    viewport.setViewedComponent (&stripHolder, false);
    viewport.setScrollBarsShown (false, true);
    viewport.setScrollBarThickness (10);
    addAndMakeVisible (viewport);
}

MixerView::~MixerView() = default;

void MixerView::selectStrip (int i)
{
    if (i <= 0 || i >= strips.size()) return;
    for (auto* st : strips) { st->selected = false; st->repaint(); }
    strips[i]->selected = true;
    selected = i;
    if (inspector) inspector->setTrack (strips[i]->number, strips[i]->name());
}

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
    header.removeFromRight (yw::captionButtonsWidth);

    inspector->setBounds (r.removeFromRight (juce::jmin (Inspector::width, r.getWidth() / 2)));
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
