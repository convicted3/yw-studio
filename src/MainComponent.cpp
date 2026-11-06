#include "MainComponent.h"
#include "LookAndFeel.h"

using namespace yw;
using namespace cmd;

void Splitter::paint (juce::Graphics& g)
{
    g.fillAll (theme::splitter);
}

//==============================================================================
MainComponent::MainComponent (AppState& s)
    : state (s), topBar (s, menuModel), browser (s), channelRack (s), mixer (s), playlist (s)
{
    menuModel.onCommand = [this] (int id) { handleCommand (id); };
    menuModel.isEnabled = [this] (int id) { return isCommandEnabled (id); };
    menuModel.isTicked  = [this] (int id) { return isCommandTicked (id); };

    for (auto* c : std::initializer_list<juce::Component*> { &topBar, &browser, &channelRack, &mixer, &playlist,
                                                            &browserSplit, &middleSplit, &rackSplit })
        addAndMakeVisible (c);

    floatingLayer.setInterceptsMouseClicks (false, true);
    addAndMakeVisible (floatingLayer);

    pianoRoll = std::make_unique<PanelWindow> ("Piano roll", std::make_unique<ComingSoonView> ("Piano roll"));
    pianoRoll->onClose = [this] { refreshUi(); };
    floatingLayer.addChildComponent (*pianoRoll);

    topBar.onCommand = [this] (int id) { handleCommand (id); };
    topBar.onShortcut = [this] (int i)
    {
        switch (i)
        {
            case TopBar::scPlaylist:     togglePanel (pPlaylist); break;
            case TopBar::scPianoRoll:    toggleFloating (*pianoRoll); break;
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
            case TopBar::scPlaylist:    return visible[pPlaylist];
            case TopBar::scPianoRoll:   return pianoRoll->isVisible();
            case TopBar::scChannelRack: return visible[pChannelRack];
            case TopBar::scMixer:       return visible[pMixer];
            case TopBar::scBrowser:     return visible[pBrowser];
            default:                    return false;
        }
    };

    browser.onClose     = [this] { togglePanel (pBrowser); };
    channelRack.onClose = [this] { togglePanel (pChannelRack); };
    mixer.onClose       = [this] { togglePanel (pMixer); };
    playlist.onClose    = [this] { togglePanel (pPlaylist); };

    browserSplit.onStart = [this] { dragStartValue = browserW; };
    browserSplit.onDrag  = [this] (int d) { browserW = juce::jlimit (160, 700, dragStartValue + d); resized(); };
    middleSplit.onStart  = [this] { dragStartValue = middleW; };
    middleSplit.onDrag   = [this] (int d) { middleW = juce::jlimit (300, 1400, dragStartValue + d); resized(); };
    rackSplit.onStart    = [this] { dragStartValue = rackH; };
    rackSplit.onDrag     = [this] (int d) { rackH = juce::jlimit (80, 2000, dragStartValue + d); resized(); };

    setSize (1600, 900);
    refreshUi();
}

MainComponent::~MainComponent()
{
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
    floatingLayer.setBounds (r);

    auto place = [] (juce::Component& c, bool show, juce::Rectangle<int> b)
    {
        c.setVisible (show);
        if (show) c.setBounds (b);
    };

    // browser | middle (channel rack over mixer) | playlist
    const bool showBrowser = visible[pBrowser];
    place (browser, showBrowser, r.removeFromLeft (showBrowser ? juce::jmin (browserW, r.getWidth() / 2) : 0));
    place (browserSplit, showBrowser, r.removeFromLeft (showBrowser ? 4 : 0));

    const bool showRack = visible[pChannelRack], showMixer = visible[pMixer], showPlaylist = visible[pPlaylist];
    const bool showMiddle = showRack || showMixer;

    juce::Rectangle<int> middle;
    if (showMiddle)
        middle = showPlaylist ? r.removeFromLeft (juce::jmin (middleW, r.getWidth() - 200)) : r;
    place (middleSplit, showMiddle && showPlaylist, showMiddle && showPlaylist ? r.removeFromLeft (4) : juce::Rectangle<int>());

    if (showRack && showMixer)
    {
        place (channelRack, true, middle.removeFromTop (juce::jmin (rackH, middle.getHeight() - 120)));
        place (rackSplit, true, middle.removeFromTop (4));
        place (mixer, true, middle);
    }
    else
    {
        place (rackSplit, false, {});
        place (channelRack, showRack, middle);
        place (mixer, showMixer, middle);
    }

    place (playlist, showPlaylist, r);

    if (! pianoRoll->getBounds().isEmpty())
        pianoRoll->setBounds (pianoRoll->getBounds().constrainedWithin (floatingLayer.getLocalBounds()));
    else
        pianoRoll->setBounds (floatingLayer.getLocalBounds().reduced (60, 40));
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
        case viewPlaylist:    return visible[pPlaylist];
        case viewChannelRack: return visible[pChannelRack];
        case viewPianoRoll:   return pianoRoll->isVisible();
        case viewMixer:       return visible[pMixer];
        case viewBrowser:     return visible[pBrowser];
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
            if (! visible[pBrowser]) togglePanel (pBrowser);
            browser.showPluginDatabase();
            break;

        case viewPlaylist:       togglePanel (pPlaylist); break;
        case viewChannelRack:    togglePanel (pChannelRack); break;
        case viewPianoRoll:      toggleFloating (*pianoRoll); break;
        case viewMixer:          togglePanel (pMixer); break;
        case viewBrowser:        togglePanel (pBrowser); break;
        case viewCloseAll:       closeAllWindows(); break;
        case viewArrangeDefault:
        case viewArrangeRealign:
            visible = { true, true, true, true };
            browserW = 325; middleW = 500; rackH = 360;
            pianoRoll->setVisible (false);
            resized();
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
            if (! visible[pBrowser]) togglePanel (pBrowser);
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
    if (is (KP::F7Key))          { toggleFloating (*pianoRoll); return true; }
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
    visible[(size_t) p] = ! visible[(size_t) p];
    resized();
    refreshUi();
}

void MainComponent::toggleFloating (PanelWindow& w)
{
    if (w.isVisible() && floatingLayer.getChildComponent (floatingLayer.getNumChildComponents() - 1) == &w)
        w.setVisible (false);
    else
    {
        w.setVisible (true);
        w.toFront (true);
    }
    refreshUi();
}

void MainComponent::closeAllWindows()
{
    pianoRoll->setVisible (false);
    visible[pChannelRack] = visible[pMixer] = false;
    resized();
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
