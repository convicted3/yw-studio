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
        explicit NameButton (ChannelInfo::Ptr i) : info (std::move (i)) {}
        juce::String getTooltip() override
        {
            return info->name + (info->sample.existsAsFile() ? " - " + info->sample.getFullPathName() : juce::String (" - no sample"))
                   + "  (click to open, right-click for options, drop a sample to replace)";
        }
        void paint (juce::Graphics& g) override
        {
            auto r = getLocalBounds().toFloat().reduced (0.5f);
            const bool over = isMouseOver() || highlighted;
            g.setGradientFill (juce::ColourGradient (juce::Colour (over ? 0xff4a5357 : 0xff424a4e), 0, r.getY(),
                                                     juce::Colour (over ? 0xff353c40 : 0xff30373b), 0, r.getBottom(), false));
            g.fillRoundedRectangle (r, 3.0f);
            g.setColour (highlighted ? theme::accent : theme::outline);
            g.drawRoundedRectangle (r, 3.0f, highlighted ? 1.5f : 1.0f);
            g.setColour (info->sample.existsAsFile() || info->name == "Sampler" ? theme::text : theme::textDim);
            g.setFont (theme::font (16.0f));
            g.drawText (info->name, getLocalBounds().reduced (6, 0), juce::Justification::centred, true);
        }
        void mouseEnter (const juce::MouseEvent&) override { repaint(); }
        void mouseExit (const juce::MouseEvent&) override  { repaint(); }
        void mouseUp (const juce::MouseEvent& e) override
        {
            if (! e.mods.isPopupMenu() && e.mouseWasClicked() && onOpen) onOpen();
        }
        void mouseDown (const juce::MouseEvent& e) override
        {
            if (! e.mods.isPopupMenu()) return;
            juce::PopupMenu m;
            m.addItem (1, "Piano roll", false);
            m.addItem (2, "Graph editor", false);
            m.addSeparator();
            m.addItem (3, "Rename...  F2");
            m.addItem (4, "Load sample...");
            m.addSeparator();
            m.addItem (5, "Insert", false);
            m.addItem (6, "Replace", false);
            m.addItem (7, "Clone");
            m.addItem (8, "Delete...");
            m.addSeparator();
            m.addItem (9, "Fill each 2 steps");
            m.addItem (10, "Fill each 4 steps");
            m.addItem (11, "Fill each 8 steps");
            m.addSeparator();
            m.addItem (12, "Rotate left   Shift+Ctrl+Left");
            m.addItem (13, "Rotate right  Shift+Ctrl+Right");
            m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [this] (int r) { if (onMenu) onMenu (r); });
        }
        ChannelInfo::Ptr info;
        bool highlighted = false;
        std::function<void (int)> onMenu;
        std::function<void()> onOpen;
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
    explicit ChannelRow (ChannelInfo::Ptr i) : info (std::move (i)), nameButton (info)
    {
        for (auto* c : std::initializer_list<juce::Component*> { &led, &pan, &vol, &track, &nameButton, &selector, &strip })
            addAndMakeVisible (c);

        vol.setDisplay ([] (float v) { return v <= 0.0001f ? juce::String ("-inf dB")
                                                         : juce::String (juce::Decibels::gainToDecibels (v / 0.78f) - 5.2f, 1) + " dB"; });
        vol.onChange = [this] (float v) { info->volume = v; };
        pan.onChange = [this] (float v) { info->pan = v; };
        syncFromInfo();
    }

    void syncFromInfo()
    {
        vol.setValue (info->volume, false);
        pan.setValue (info->pan, false);
        track.track = info->mixerTrack;
        nameButton.repaint();
        track.repaint();
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
        nameButton.setBounds (r.removeFromLeft (100).reduced (0, 1));
        r.removeFromLeft (6);
        selector.setBounds (r.removeFromLeft (12));
        r.removeFromLeft (10);
        strip.setBounds (r.removeFromLeft (juce::jmin (r.getWidth() - 8, 16 * 22)).reduced (0, 1));
    }

    ChannelInfo::Ptr info;
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
        m.addItem (1, "Add one (Sampler)");
        m.addItem (9, "Load samples...");
        m.addItem (2, "Select unused channels", false);
        m.addItem (3, "Clone selected  Alt+C");
        m.addItem (4, "Delete selected  Alt+Del");
        m.addSeparator();
        m.addItem (5, "Group selected  Alt+G", false);
        m.addItem (6, "Zip selected  Alt+Z", false);
        m.addSeparator();
        m.addItem (7, "Detached", false);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&menuButton), [this] (int r)
        {
            if (r == 1) addChannel (std::make_shared<ChannelInfo>());
            if (r == 9) loadSampleInto (-1);
            if (r == 3) for (int i = rows.size(); --i >= 0;) if (rows[i]->selector.selected) cloneChannel (i);
            if (r == 4) for (int i = rows.size(); --i >= 0;) if (rows[i]->selector.selected) removeChannel (i);
        });
    };

    filter.addItem ("All", 1);
    filter.addItem ("Unsorted", 2);
    filter.setSelectedId (1, juce::dontSendNotification);
    filter.setTooltip ("Channel display filter");
    lengthBox.setTooltip ("Pattern length (auto)");
    stepsButton.setToggleState (false, juce::dontSendNotification);
    addButton.setIconScale (0.55f);
    addButton.onClick = [this]
    {
        juce::PopupMenu m;
        m.addItem (1, "Sampler");
        m.addItem (3, "Sampler from file...");
        m.addSeparator();
        int id = 100;
        for (auto& d : state.knownPlugins.getTypes())
            if (d.isInstrument) m.addItem (id++, d.name, false);
        if (id == 100) m.addItem (2, "More plugins... (Options > Manage plugins)", false);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&addButton), [this] (int r)
        {
            if (r == 1) addChannel (std::make_shared<ChannelInfo>());
            if (r == 3) loadSampleInto (-1);
        });
    };

    for (auto* c : std::initializer_list<juce::Component*> { &menuButton, &undoButton, &filter, &swing, &lengthBox,
                                                            &graphButton, &stepsButton })
        addAndMakeVisible (c);

    rowHolder.addAndMakeVisible (addButton);
    viewport.setViewedComponent (&rowHolder, false);
    viewport.setScrollBarsShown (true, false);
    viewport.setScrollBarThickness (10);
    addAndMakeVisible (viewport);

    addChannel (std::make_shared<ChannelInfo>());
}

ChannelRackView::~ChannelRackView() = default;

void ChannelRackView::addChannel (ChannelInfo::Ptr info, int index)
{
    auto* row = new ChannelRow (info);
    if (index < 0 || index > rows.size()) index = rows.size();
    rows.insert (index, row);
    rowHolder.addAndMakeVisible (row);

    row->nameButton.onOpen = [this, row] { if (onOpenChannel) onOpenChannel (row->info); };
    row->nameButton.onMenu = [this, row] (int r)
    {
        const int idx = rows.indexOf (row);
        auto& st = row->strip.steps;
        if (r == 3) renameChannel (idx);
        if (r == 4) loadSampleInto (idx);
        if (r == 7) cloneChannel (idx);
        if (r == 8) { removeChannel (idx); return; }
        if (r >= 9 && r <= 11) { const int every = r == 9 ? 2 : r == 10 ? 4 : 8; for (size_t i = 0; i < st.size(); ++i) st[i] = i % (size_t) every == 0; }
        if (r == 12) std::rotate (st.begin(), st.begin() + 1, st.end());
        if (r == 13) std::rotate (st.rbegin(), st.rbegin() + 1, st.rend());
        row->strip.repaint();
    };
    resized();
    repaint();
}

void ChannelRackView::removeChannel (int index)
{
    if (! juce::isPositiveAndBelow (index, rows.size())) return;
    rows.remove (index);
    resized();
    repaint();
}

void ChannelRackView::cloneChannel (int index)
{
    if (! juce::isPositiveAndBelow (index, rows.size())) return;
    auto copy = std::make_shared<ChannelInfo> (*rows[index]->info);
    copy->name << " #2";
    addChannel (copy, index + 1);
    rows[index + 1]->strip.steps = rows[index]->strip.steps;
}

void ChannelRackView::renameChannel (int index)
{
    if (! juce::isPositiveAndBelow (index, rows.size())) return;
    auto info = rows[index]->info;
    auto* w = new juce::AlertWindow ("Rename channel", "Name:", juce::MessageBoxIconType::NoIcon, this);
    w->addTextEditor ("name", info->name);
    w->addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    w->enterModalState (true, juce::ModalCallbackFunction::create ([this, w, info] (int r)
    {
        if (r == 1 && w->getTextEditorContents ("name").trim().isNotEmpty())
        {
            info->name = w->getTextEditorContents ("name").trim();
            refreshRows();
            if (onChannelChanged) onChannelChanged (info);
        }
    }), true);
}

void ChannelRackView::loadSampleInto (int index)
{
    chooser = std::make_unique<juce::FileChooser> ("Load sample", juce::File::getSpecialLocation (juce::File::userMusicDirectory),
                                                   SampleLibrary::audioWildcard());
    const int flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles
                      | (index < 0 ? juce::FileBrowserComponent::canSelectMultipleItems : 0);
    chooser->launchAsync (flags, [this, index] (const juce::FileChooser& fc)
    {
        auto files = fc.getResults();
        if (files.isEmpty()) return;
        if (juce::isPositiveAndBelow (index, rows.size()))
        {
            auto info = rows[index]->info;
            info->sample = files[0];
            info->name = files[0].getFileNameWithoutExtension();
            refreshRows();
            if (onChannelChanged) onChannelChanged (info);
        }
        else addSampleChannels (files);
    });
}

void ChannelRackView::addSampleChannels (const juce::Array<juce::File>& files, int insertIndex)
{
    for (auto& f : files)
    {
        if (! SampleLibrary::isAudioFile (f)) continue;
        auto info = std::make_shared<ChannelInfo>();
        info->sample = f;
        info->name = f.getFileNameWithoutExtension();
        info->mixerTrack = juce::jmin (125, rows.size() + 1);
        addChannel (info, insertIndex);
        if (insertIndex >= 0) ++insertIndex;
    }
}

void ChannelRackView::sendToSelected (const juce::File& file)
{
    for (auto* row : rows)
        if (row->selector.selected)
        {
            row->info->sample = file;
            row->info->name = file.getFileNameWithoutExtension();
            row->syncFromInfo();
            if (onChannelChanged) onChannelChanged (row->info);
            return;
        }
    addSampleChannels ({ file });
}

void ChannelRackView::refreshRows()
{
    for (auto* r : rows) r->syncFromInfo();
}

//==============================================================================
juce::Array<juce::File> ChannelRackView::audioFilesFrom (const juce::StringArray& paths)
{
    juce::Array<juce::File> out;
    for (auto& p : paths)
        if (SampleLibrary::isAudioFile (juce::File (p))) out.add (juce::File (p));
    return out;
}

int ChannelRackView::rowIndexAt (juce::Point<int> p) const
{
    for (int i = 0; i < rows.size(); ++i)
        if (rows[i]->getScreenBounds().contains (localPointToGlobal (p))) return i;
    return -1;
}

void ChannelRackView::updateDropTarget (juce::Point<int> p, bool active)
{
    const int newRow = active ? rowIndexAt (p) : -1;
    if (newRow != dropRow || active != dropActive)
    {
        if (juce::isPositiveAndBelow (dropRow, rows.size())) { rows[dropRow]->nameButton.highlighted = false; rows[dropRow]->nameButton.repaint(); }
        dropRow = newRow;
        dropActive = active;
        if (juce::isPositiveAndBelow (dropRow, rows.size())) { rows[dropRow]->nameButton.highlighted = true; rows[dropRow]->nameButton.repaint(); }
        repaint();
    }
}

void ChannelRackView::dropFiles (const juce::Array<juce::File>& files, juce::Point<int> p)
{
    const int target = rowIndexAt (p);
    updateDropTarget (p, false);
    if (files.isEmpty()) return;

    if (target >= 0 && files.size() == 1)
    {
        auto info = rows[target]->info;
        info->sample = files[0];
        info->name = files[0].getFileNameWithoutExtension();
        rows[target]->syncFromInfo();
        if (onChannelChanged) onChannelChanged (info);
    }
    else addSampleChannels (files, target >= 0 ? target + 1 : -1);
}

bool ChannelRackView::isInterestedInDragSource (const SourceDetails& d)
{
    return SampleLibrary::isAudioFile (juce::File (d.description.toString()));
}
void ChannelRackView::itemDragMove (const SourceDetails& d) { updateDropTarget (d.localPosition, true); }
void ChannelRackView::itemDragExit (const SourceDetails&)   { updateDropTarget ({}, false); }
void ChannelRackView::itemDropped (const SourceDetails& d)
{
    dropFiles ({ juce::File (d.description.toString()) }, d.localPosition);
}

bool ChannelRackView::isInterestedInFileDrag (const juce::StringArray& files) { return ! audioFilesFrom (files).isEmpty(); }
void ChannelRackView::fileDragMove (const juce::StringArray&, int x, int y)  { updateDropTarget ({ x, y }, true); }
void ChannelRackView::fileDragExit (const juce::StringArray&)                { updateDropTarget ({}, false); }
void ChannelRackView::filesDropped (const juce::StringArray& files, int x, int y)
{
    dropFiles (audioFilesFrom (files), { x, y });
}

//==============================================================================
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

    // rack background behind the rows
    auto rack = viewport.getBounds().withHeight (juce::jmin (viewport.getHeight(), rows.size() * rowHeight + 8));
    g.setColour (theme::panelBody);
    g.fillRoundedRectangle (rack.toFloat(), 3.0f);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawRoundedRectangle (rack.toFloat(), 3.0f, 1.0f);
}

void ChannelRackView::paintOverChildren (juce::Graphics& g)
{
    if (! dropActive) return;
    if (dropRow < 0)
    {
        // new channel: insertion line below the last row
        const int y = viewport.getY() + juce::jmin (viewport.getHeight() - 4, rows.size() * rowHeight + 4 - viewport.getViewPositionY());
        g.setColour (theme::accent);
        g.fillRoundedRectangle ((float) viewport.getX() + 6.0f, (float) y, (float) viewport.getWidth() - 20.0f, 3.0f, 1.5f);
        g.setColour (theme::textBright);
        g.setFont (theme::font (15.0f));
        g.drawText ("Drop to add as new channel(s)", viewport.getX() + 10, y + 6, viewport.getWidth() - 20, 18, juce::Justification::centredLeft);
    }
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

    viewport.setBounds (r.reduced (6, 4));
    const int w = viewport.getWidth() - (rows.size() * rowHeight + 40 > viewport.getHeight() ? viewport.getScrollBarThickness() : 0);
    int y = 4;
    for (auto* row : rows) { row->setBounds (2, y, w - 4, rowHeight); y += rowHeight; }
    y += 8;
    addButton.setBounds (150 - 13 + 2, y, 26, 24);
    rowHolder.setSize (w, y + 32);
}
