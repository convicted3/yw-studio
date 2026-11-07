#include "SamplerView.h"
#include "LookAndFeel.h"

using namespace yw;

SamplerView::SamplerView (AppState& s) : state (s)
{
    prevPreset.setIcon (icons::Id::arrowLeft);
    prevPreset.onClick = [this]
    {
        if (! info || ! info->sample.existsAsFile()) return;
        auto files = info->sample.getParentDirectory().findChildFiles (juce::File::findFiles, false);
        files.sort();
        files.removeIf ([] (const juce::File& f) { return ! SampleLibrary::isAudioFile (f); });
        const int i = files.indexOf (info->sample);
        if (i > 0) setSample (files[i - 1]);
    };
    nextPreset.onClick = [this]
    {
        if (! info || ! info->sample.existsAsFile()) return;
        auto files = info->sample.getParentDirectory().findChildFiles (juce::File::findFiles, false);
        files.sort();
        files.removeIf ([] (const juce::File& f) { return ! SampleLibrary::isAudioFile (f); });
        const int i = files.indexOf (info->sample);
        if (i >= 0 && i + 1 < files.size()) setSample (files[i + 1]);
    };

    pan.setStyle (true);
    volume.setStyle (true);
    pitch.setStyle (true);
    pan.onChange    = [this] (float v) { if (info) { info->pan = v;    if (onChanged) onChanged (info); } };
    volume.onChange = [this] (float v) { if (info) { info->volume = v; if (onChanged) onChanged (info); } };
    pitch.onChange  = [this] (float v) { if (info) info->pitch = v; };
    pitch.setDisplay ([] (float v) { return juce::String (juce::roundToInt ((v - 0.5f) * 4800.0f)) + " cents"; });

    for (auto t : { "SMP", "INS", "ART", "FUNC" })
    {
        auto* b = tabs.add (new juce::TextButton (t));
        b->setRadioGroupId (51);
        b->setClickingTogglesState (true);
        addAndMakeVisible (b);
    }
    tabs[0]->setToggleState (true, juce::dontSendNotification);
    for (int i = 1; i < tabs.size(); ++i) tabs[i]->setEnabled (false);

    openButton.onClick = [this] { chooseSample(); };
    reverse.onClick = [this] { if (info) info->reverse = reverse.getToggleState(); repaint(); };
    loop.onClick    = [this] { if (info) info->loop = loop.getToggleState(); };

    for (auto* c : std::initializer_list<juce::Component*> { &menuButton, &prevPreset, &nextPreset, &pan, &volume, &pitch,
                                                            &openButton, &reverse, &loop })
        addAndMakeVisible (c);

    menuButton.onClick = [this]
    {
        juce::PopupMenu m;
        m.addItem (1, "Load sample...");
        m.addItem (2, "Show sample in Explorer", info && info->sample.existsAsFile());
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&menuButton), [this] (int r)
        {
            if (r == 1) chooseSample();
            if (r == 2 && info) info->sample.revealToUser();
        });
    };

    startTimerHz (10);
}

void SamplerView::setChannel (ChannelInfo::Ptr i)
{
    info = std::move (i);
    if (info)
    {
        pan.setValue (info->pan, false);
        volume.setValue (info->volume, false);
        pitch.setValue (info->pitch, false);
        reverse.setToggleState (info->reverse, juce::dontSendNotification);
        loop.setToggleState (info->loop, juce::dontSendNotification);
    }
    repaint();
}

void SamplerView::setSample (const juce::File& f)
{
    if (! info || ! SampleLibrary::isAudioFile (f)) return;
    info->sample = f;
    info->name = f.getFileNameWithoutExtension();
    repaint();
    if (onChanged) onChanged (info);
}

void SamplerView::chooseSample()
{
    auto start = info && info->sample.existsAsFile() ? info->sample.getParentDirectory()
                                                      : juce::File::getSpecialLocation (juce::File::userMusicDirectory);
    chooser = std::make_unique<juce::FileChooser> ("Load sample", start, SampleLibrary::audioWildcard());
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this] (const juce::FileChooser& fc) { if (fc.getResult().existsAsFile()) setSample (fc.getResult()); });
}

void SamplerView::timerCallback()
{
    // repaint while the waveform is still loading in the background
    if (info && info->sample.existsAsFile() && ! state.samples.thumbnail (info->sample).isFullyLoaded())
        repaint (waveArea);
}

void SamplerView::paint (juce::Graphics& g)
{
    g.fillAll (theme::panelBody);

    // caption: name of the channel
    auto header = getLocalBounds().removeFromTop (headerHeight);
    drawPanelHeader (g, header, {}, icons::Id::none);
    auto title = header.withX (nextPreset.getRight() + 8).withRight (pan.getX() - 8);
    g.setColour (theme::text);
    g.setFont (theme::font (18.0f, theme::Weight::medium));
    g.drawText (info ? info->name : juce::String ("Sampler"), title, juce::Justification::centredLeft, true);

    // knob labels
    g.setColour (theme::textDim);
    g.setFont (theme::font (11.0f, theme::Weight::semibold));
    for (auto* k : { &pan, &volume, &pitch })
    {
        const char* label = k == &pan ? "PAN" : k == &volume ? "VOL" : "PITCH";
        g.drawText (label, k->getBounds().translated (0, k->getHeight()).withHeight (12), juce::Justification::centred);
    }

    // sample info
    g.setColour (juce::Colour (0xff1e2427));
    g.fillRoundedRectangle (infoArea.toFloat(), 3.0f);
    auto ia = infoArea.reduced (8, 4);
    g.setFont (theme::font (16.0f));
    if (info && info->sample.existsAsFile())
    {
        const double len = state.samples.lengthSeconds (info->sample);
        g.setColour (theme::textBright);
        g.drawText (info->sample.getFileName(), ia.removeFromTop (ia.getHeight() / 2), juce::Justification::centredLeft, true);
        g.setColour (theme::textDim);
        g.setFont (theme::font (14.0f));
        g.drawText (juce::String (len, 2) + " s  -  " + info->sample.getParentDirectory().getFullPathName(), ia,
                    juce::Justification::centredLeft, true);
    }
    else
    {
        g.setColour (theme::textDim);
        g.drawText ("No sample - drop one here or click \"Open sample...\"", ia, juce::Justification::centredLeft, true);
    }

    // waveform
    g.setColour (juce::Colour (0xff11171a));
    g.fillRoundedRectangle (waveArea.toFloat(), 3.0f);
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.drawHorizontalLine (waveArea.getCentreY(), (float) waveArea.getX(), (float) waveArea.getRight());
    if (info && info->sample.existsAsFile())
    {
        juce::Graphics::ScopedSaveState ss (g);
        auto wr = waveArea.reduced (4).toFloat();
        if (info->reverse)
            g.addTransform (juce::AffineTransform::scale (-1.0f, 1.0f, wr.getCentreX(), wr.getCentreY()));
        state.samples.drawWaveform (g, info->sample, wr, juce::Colour (0xff8fd36a));
    }
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawRoundedRectangle (waveArea.toFloat(), 3.0f, 1.0f);

    if (dropping)
    {
        g.setColour (theme::accent.withAlpha (0.15f));
        g.fillRect (getLocalBounds().withTrimmedTop (headerHeight));
        g.setColour (theme::accent);
        g.drawRect (getLocalBounds().withTrimmedTop (headerHeight), 2);
    }
}

void SamplerView::resized()
{
    auto r = getLocalBounds();
    auto header = r.removeFromTop (headerHeight).reduced (2, 1);
    menuButton.setBounds (header.removeFromLeft (20));
    prevPreset.setBounds (header.removeFromLeft (20));
    nextPreset.setBounds (header.removeFromLeft (20));
    header.removeFromRight (yw::captionButtonsWidth);

    auto knobs = r.removeFromTop (54).reduced (10, 6);
    for (auto* k : { &pitch, &volume, &pan })
    {
        k->setBounds (knobs.removeFromRight (34).withHeight (30));
        knobs.removeFromRight (10);
    }
    tabs.isEmpty();
    auto tabRow = knobs.withTrimmedTop (4).withHeight (26);
    for (auto* t : tabs) t->setBounds (tabRow.removeFromLeft (52).reduced (1, 0));

    r = r.reduced (10, 4);
    infoArea = r.removeFromTop (42);
    r.removeFromTop (8);
    auto bottom = r.removeFromBottom (30);
    openButton.setBounds (bottom.removeFromLeft (130).reduced (0, 2));
    bottom.removeFromLeft (12);
    reverse.setBounds (bottom.removeFromLeft (100));
    loop.setBounds (bottom.removeFromLeft (80));
    r.removeFromBottom (8);
    waveArea = r;
}

bool SamplerView::isInterestedInDragSource (const SourceDetails& d)
{
    return SampleLibrary::isAudioFile (juce::File (d.description.toString()));
}

void SamplerView::itemDropped (const SourceDetails& d)
{
    dropping = false;
    setSample (juce::File (d.description.toString()));
}

bool SamplerView::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (auto& f : files) if (SampleLibrary::isAudioFile (juce::File (f))) return true;
    return false;
}

void SamplerView::filesDropped (const juce::StringArray& files, int, int)
{
    dropping = false;
    for (auto& f : files) if (SampleLibrary::isAudioFile (juce::File (f))) { setSample (juce::File (f)); break; }
}
