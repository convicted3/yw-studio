#include "MainComponent.h"
#include "LookAndFeel.h"

using namespace yw;
using namespace cmd;

//==============================================================================
void PianoRollPlaceholder::paint (juce::Graphics& g)
{
    auto r = getLocalBounds();
    auto header = r.removeFromTop (24);
    drawPanelHeader (g, header, {}, icons::Id::none);
    auto title = header.reduced (8, 0);
    icons::draw (g, icons::Id::pianoRoll, title.removeFromLeft (20).toFloat().reduced (2.0f), theme::text);
    g.setColour (theme::text);
    g.setFont (theme::font (18.0f, theme::Weight::medium));
    g.drawText ("Piano roll - Sampler", title.withTrimmedLeft (6), juce::Justification::centredLeft, true);

    // keyboard + grid preview
    auto keys = r.removeFromLeft (64);
    for (int i = 0; i * 14 < keys.getHeight(); ++i)
    {
        const bool black = juce::Array<int> { 1, 3, 6, 8, 10 }.contains (i % 12);
        g.setColour (black ? juce::Colour (0xff1a1e21) : juce::Colour (0xffd9dcdd));
        g.fillRect (keys.getX(), keys.getY() + i * 14, black ? 40 : 64, 13);
    }
    g.setColour (juce::Colour (0xff1e282e));
    g.fillRect (r);
    for (int x = r.getX(); x < r.getRight(); x += 24)
    {
        g.setColour ((x - r.getX()) % 96 == 0 ? juce::Colour (0xff00040a) : juce::Colour (0xff141e24));
        g.fillRect (x, r.getY(), 1, r.getHeight());
    }
    for (int y = r.getY(); y < r.getBottom(); y += 14) { g.setColour (juce::Colour (0xff141e24)); g.fillRect (r.getX(), y, r.getWidth(), 1); }
    g.setColour (theme::textDim);
    g.setFont (theme::font (18.0f));
    g.drawText ("Piano roll - in development", r, juce::Justification::centred);
}

//==============================================================================
MainComponent::MainComponent (AppState& s)
    : state (s), topBar (s, menuModel), browser (s), channelRack (s), mixer (s), playlist (s), samplerView (s)
{
    menuModel.onCommand = [this] (int id) { handleCommand (id); };
    menuModel.isEnabled = [this] (int id) { return isCommandEnabled (id); };
    menuModel.isTicked  = [this] (int id) { return isCommandTicked (id); };

    addAndMakeVisible (topBar);
    addAndMakeVisible (workspace);

    struct Def { Panel p; const char* id; juce::Component* view; int caption; juce::Point<int> minSize; };
    const Def defs[] = {
        { pBrowser,     "browser",     &browser,       22, { 180, 200 } },
        { pChannelRack, "channelRack", &channelRack,   24, { 360, 110 } },
        { pMixer,       "mixer",       &mixer,         24, { 420, 260 } },
        { pPlaylist,    "playlist",    &playlist,      22, { 480, 220 } },
        { pPianoRoll,   "pianoRoll",   &pianoRollView, 24, { 420, 220 } },
        { pSampler,     "sampler",     &samplerView,   24, { 420, 280 } } };

    for (auto& d : defs)
    {
        auto w = std::make_unique<DockWindow> (d.id, *d.view, d.caption, d.minSize);
        w->onVisibilityChanged = [this] { refreshUi(); };
        w->getSnapTargets = [this, raw = w.get()]
        {
            juce::Array<juce::Rectangle<int>> r;
            for (auto& o : windows) if (o && o.get() != raw && o->isVisible()) r.add (o->getBounds());
            return r;
        };
        workspace.addChildComponent (*w);
        windows[(size_t) d.p] = std::move (w);
    }

    // samples: browser -> channel rack, channel -> sampler window
    browser.onOpenInNewChannel = [this] (const juce::File& f) { channelRack.addSampleChannels ({ f }); openPanel (pChannelRack); };
    browser.onSendToSelected   = [this] (const juce::File& f) { channelRack.sendToSelected (f); };
    channelRack.onOpenChannel = [this] (ChannelInfo::Ptr info)
    {
        auto& w = window (pSampler);
        if (w.isVisible() && samplerView.getChannel() == info) w.close();
        else { samplerView.setChannel (info); w.open(); }
    };
    channelRack.onChannelChanged = [this] (ChannelInfo::Ptr info) { if (samplerView.getChannel() == info) samplerView.setChannel (info); };
    samplerView.onChanged = [this] (ChannelInfo::Ptr) { channelRack.refreshRows(); };

    topBar.onCommand = [this] (int id) { handleCommand (id); };
    topBar.onShortcut = [this] (int i)
    {
        switch (i)
        {
            case TopBar::scPlaylist:     togglePanel (pPlaylist); break;
            case TopBar::scPianoRoll:    togglePanel (pPianoRoll); break;
            case TopBar::scChannelRack:  togglePanel (pChannelRack); break;
            case TopBar::scMixer:        togglePanel (pMixer); break;
            case TopBar::scBrowser:      togglePanel (pBrowser); break;
            case TopBar::scProject:      showSettings (SettingsWindow::project); break;
            case TopBar::scPluginPicker: showPluginManager(); break;
            case TopBar::scCatalog:      handleCommand (toolPluginCatalog); break;
            default: break;
        }
    };
    topBar.isShortcutActive = [this] (int i)
    {
        switch (i)
        {
            case TopBar::scPlaylist:    return window (pPlaylist).isVisible();
            case TopBar::scPianoRoll:   return window (pPianoRoll).isVisible();
            case TopBar::scChannelRack: return window (pChannelRack).isVisible();
            case TopBar::scMixer:       return window (pMixer).isVisible();
            case TopBar::scBrowser:     return window (pBrowser).isVisible();
            default:                    return false;
        }
    };

    setSize (1600, 900);
    refreshUi();
}

MainComponent::~MainComponent()
{
    saveLayout();
    menuModel.onCommand = nullptr;
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (theme::workspace);
}

void MainComponent::resized()
{
    auto r = getLocalBounds();
    topBar.setBounds (r.removeFromTop (TopBar::height));
    workspace.setBounds (r);

    // first real layout: restore the saved window layout or build the default one
    if (! layoutDone && workspace.getWidth() > 600 && workspace.getHeight() > 300 && isShowing())
    {
        layoutDone = true;
        if (! restoreLayout()) arrangeDefault();
        refreshUi();
    }
}

void MainComponent::arrangeDefault()
{
    auto r = workspace.getLocalBounds();
    const int gap = 1;
    const int browserW = juce::jmin (325, r.getWidth() / 5);
    const int middleW  = juce::jmin (500, (r.getWidth() - browserW) / 2);
    const int rackH    = juce::jmin (360, r.getHeight() / 2);

    for (auto& w : windows) w->setMaximised (false);

    window (pBrowser).setBounds (r.removeFromLeft (browserW));
    r.removeFromLeft (gap);
    auto middle = r.removeFromLeft (middleW);
    r.removeFromLeft (gap);
    window (pChannelRack).setBounds (middle.removeFromTop (rackH));
    middle.removeFromTop (gap);
    window (pMixer).setBounds (middle);
    window (pPlaylist).setBounds (r);
    window (pPianoRoll).setBounds (workspace.getLocalBounds().reduced (workspace.getWidth() / 8, workspace.getHeight() / 8));

    for (auto p : { pBrowser, pChannelRack, pMixer, pPlaylist }) window (p).setVisible (true);
    window (pPianoRoll).setVisible (false);
    window (pSampler).setBounds (juce::Rectangle<int> (workspace.getWidth() / 2 - 300, 60, 600, 420).constrainedWithin (workspace.getLocalBounds()));
    window (pSampler).setVisible (false);
    window (pChannelRack).toFront (false);
}

void MainComponent::saveLayout()
{
    if (! layoutDone) return;
    juce::XmlElement xml ("WINDOWS");
    for (auto& w : windows)
    {
        auto* e = xml.createNewChildElement ("WINDOW");
        e->setAttribute ("id", w->getId());
        e->setAttribute ("bounds", w->getNormalBounds().toString());
        e->setAttribute ("visible", w->isVisible());
        e->setAttribute ("maximised", w->isMaximised());
        e->setAttribute ("z", workspace.getIndexOfChildComponent (w.get()));
    }
    state.settings().setValue ("windowLayout", &xml);
    state.saveSettings();
}

bool MainComponent::restoreLayout()
{
    auto xml = state.settings().getXmlValue ("windowLayout");
    if (xml == nullptr) return false;

    juce::Array<std::pair<int, DockWindow*>> order;
    int found = 0;
    for (auto* e : xml->getChildIterator())
        for (auto& w : windows)
            if (w->getId() == e->getStringAttribute ("id"))
            {
                auto b = juce::Rectangle<int>::fromString (e->getStringAttribute ("bounds"));
                if (b.isEmpty()) continue;
                b.setSize (juce::jmin (b.getWidth(), workspace.getWidth()), juce::jmin (b.getHeight(), workspace.getHeight()));
                w->setBounds (b.constrainedWithin (workspace.getLocalBounds()));
                w->setVisible (e->getBoolAttribute ("visible", true));
                if (e->getBoolAttribute ("maximised")) w->setMaximised (true);
                order.add ({ e->getIntAttribute ("z"), w.get() });
                ++found;
            }

    std::sort (order.begin(), order.end(), [] (auto& a, auto& b) { return a.first < b.first; });
    for (auto& o : order) o.second->toFront (false);
    return found == (int) windows.size();
}

void MainComponent::refreshUi()
{
    topBar.refresh();
    menuModel.menuItemsChanged();
}

//==============================================================================
bool MainComponent::isCommandEnabled (int id) const
{
    switch (id)
    {
        case fileExit: case helpIndex: case helpUpdates: case helpWebsite: case helpGithub: case helpAbout:
        case addRefreshPlugins: case addManagePlugins: case optManagePlugins: case addPluginPicker: case viewPluginPicker:
        case addBrowseLibrary: case addBrowseInstalled: case viewBrowserLibrary:
        case viewPlaylist: case viewChannelRack: case viewPianoRoll: case viewMixer: case viewBrowser:
        case viewCloseAll: case viewArrangeDefault: case viewArrangeRealign:
        case optMidi: case optAudio: case optGeneral: case optFile: case optProjectGeneral: case optProjectInfo:
        case toolFind: case toolPanicStop: case patFindNextEmpty: case patFindFirstEmpty:
            return true;
        default:
            return false;
    }
}

bool MainComponent::isCommandTicked (int id) const
{
    switch (id)
    {
        case viewPlaylist:    return window (pPlaylist).isVisible();
        case viewChannelRack: return window (pChannelRack).isVisible();
        case viewPianoRoll:   return window (pPianoRoll).isVisible();
        case viewMixer:       return window (pMixer).isVisible();
        case viewBrowser:     return window (pBrowser).isVisible();
        default:              return false;
    }
}

void MainComponent::handleCommand (int id)
{
    switch (id)
    {
        case fileExit:           juce::JUCEApplication::getInstance()->systemRequestedQuit(); break;

        case addRefreshPlugins: case addManagePlugins: case optManagePlugins:
        case addPluginPicker: case viewPluginPicker:
                                 showPluginManager(); break;

        case addBrowseLibrary: case addBrowseInstalled: case viewBrowserLibrary:
            openPanel (pBrowser);
            browser.showPluginDatabase();
            break;

        case viewPlaylist:       togglePanel (pPlaylist); break;
        case viewChannelRack:    togglePanel (pChannelRack); break;
        case viewPianoRoll:      togglePanel (pPianoRoll); break;
        case viewMixer:          togglePanel (pMixer); break;
        case viewBrowser:        togglePanel (pBrowser); break;
        case viewCloseAll:       closeAllWindows(); break;
        case viewArrangeDefault:
        case viewArrangeRealign:
            arrangeDefault();
            break;

        case patFindNextEmpty:
        case patFindFirstEmpty:
            state.numPatterns += 1; state.currentPattern = state.numPatterns;
            repaint();
            break;

        case optMidi:            showSettings (SettingsWindow::midi); break;
        case optAudio:           showSettings (SettingsWindow::audio); break;
        case optGeneral:         showSettings (SettingsWindow::general); break;
        case optFile:            showSettings (SettingsWindow::file); break;
        case optProjectGeneral:
        case optProjectInfo:     showSettings (SettingsWindow::project); break;

        case toolFind:
            openPanel (pBrowser);
            browser.focusSearch();
            break;
        case toolPanicStop:      topBar.stop(); break;

        case helpIndex:
        case helpWebsite:        juce::URL ("https://yolo.su").launchInDefaultBrowser(); break;
        case helpUpdates:        juce::URL ("https://github.com/convicted3/yw-studio/releases").launchInDefaultBrowser(); break;
        case helpGithub:         juce::URL ("https://github.com/convicted3/yw-studio/issues").launchInDefaultBrowser(); break;
        case helpAbout:          showAbout(); break;
        default: break;
    }

    refreshUi();
}

bool MainComponent::keyPressed (const juce::KeyPress& k, juce::Component*)
{
    using KP = juce::KeyPress;
    const auto mods = k.getModifiers().withoutMouseButtons();
    const int code = k.getKeyCode();
    auto is = [&] (int key, juce::ModifierKeys m = {}) { return code == key && mods == m; };
    const juce::ModifierKeys ctrl (juce::ModifierKeys::commandModifier), alt (juce::ModifierKeys::altModifier),
                             shift (juce::ModifierKeys::shiftModifier);

    if (is (KP::F5Key))          { togglePanel (pPlaylist); return true; }
    if (is (KP::F6Key))          { togglePanel (pChannelRack); return true; }
    if (is (KP::F7Key))          { togglePanel (pPianoRoll); return true; }
    if (is (KP::F8Key))          { showPluginManager(); return true; }
    if (is (KP::F8Key, alt))     { togglePanel (pBrowser); return true; }
    if (is (KP::F8Key, shift))   { handleCommand (viewBrowserLibrary); return true; }
    if (is (KP::F9Key))          { togglePanel (pMixer); return true; }
    if (is (KP::F10Key))         { showSettings (SettingsWindow::midi); return true; }
    if (is (KP::F11Key))         { showSettings (SettingsWindow::project); return true; }
    if (is (KP::F12Key))         { closeAllWindows(); return true; }
    if (is (KP::F1Key))          { handleCommand (helpIndex); return true; }
    if (is (KP::F4Key))          { handleCommand (patFindNextEmpty); return true; }
    if (is (KP::spaceKey))       { if (state.transport.playing) topBar.stop(); else topBar.togglePlay(); return true; }
    if (is ('L') || is ('l'))    { topBar.togglePatSong(); return true; }
    if (is ('F', ctrl))          { handleCommand (toolFind); return true; }
    if (is ('H', ctrl))          { topBar.stop(); return true; }

    return false;
}

//==============================================================================
void MainComponent::togglePanel (Panel p)
{
    window (p).toggle();
    refreshUi();
}

void MainComponent::openPanel (Panel p)
{
    window (p).open();
    refreshUi();
}

void MainComponent::closeAllWindows()
{
    for (auto p : { pChannelRack, pMixer, pPianoRoll, pSampler }) window (p).close();
    refreshUi();
}

void MainComponent::showPluginManager()
{
    if (! pluginManager) pluginManager = std::make_unique<PluginManagerWindow> (state);
    pluginManager->setVisible (true);
    pluginManager->toFront (true);
}

void MainComponent::showSettings (SettingsWindow::Tab tab)
{
    if (! settingsWindow) settingsWindow = std::make_unique<SettingsWindow> (state);
    settingsWindow->showTab (tab);
}

void MainComponent::showAbout()
{
    juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::NoIcon, "About YW Studio",
        juce::String ("YW Studio ") + AppState::version + "\n\n"
        "Free and open-source digital audio workstation by #YW.\n"
        "https://yolo.su\n\n"
        "Licensed under the GNU AGPL v3. Built with JUCE.\n"
        "YW Studio is an independent project and is not affiliated with Image-Line.");
}
