#include "MainComponent.h"
#include "LookAndFeel.h"

using namespace yw;
using namespace cmd;

//==============================================================================
void Workspace::paint (juce::Graphics& g)
{
    g.fillAll (theme::bgDeep);

    // Faint YW watermark in the empty desktop
    static const auto logo = juce::ImageCache::getFromMemory (BinaryData::yw_logo_png, BinaryData::yw_logo_pngSize);
    if (logo.isValid())
    {
        g.setOpacity (0.04f);
        g.drawImage (logo, getLocalBounds().toFloat().withSizeKeepingCentre (360.0f, 230.0f),
                     juce::RectanglePlacement::centred);
    }
}

//==============================================================================
TitleRow::TitleRow (juce::MenuBarModel& model) : menuBar (&model)
{
    logo = juce::ImageCache::getFromMemory (BinaryData::yw_logo_png, BinaryData::yw_logo_pngSize);
    addAndMakeVisible (menuBar);
    startTimerHz (15);
}

void TitleRow::paint (juce::Graphics& g)
{
    g.fillAll (theme::bgRaised);

    auto r = getLocalBounds();
    if (logo.isValid())
        g.drawImage (logo, r.removeFromLeft (44).reduced (8, 6).toFloat(), juce::RectanglePlacement::centred);

    // Hint bar (right part): shows the tooltip of whatever is under the mouse
    auto hintArea = getLocalBounds().withTrimmedLeft (menuBar.getRight() + 12).reduced (4, 5);
    g.setColour (theme::bgDeep);
    g.fillRoundedRectangle (hintArea.toFloat(), 3.0f);
    g.setColour (theme::textDim);
    g.setFont (theme::font (12.0f));
    g.drawText (hint, hintArea.reduced (8, 0), juce::Justification::centredLeft, true);
}

void TitleRow::resized()
{
    auto r = getLocalBounds().withTrimmedLeft (44);
    menuBar.setBounds (r.removeFromLeft (560));
}

void TitleRow::timerCallback()
{
    juce::String newHint;
    auto* c = juce::Desktop::getInstance().getMainMouseSource().getComponentUnderMouse();
    for (; c != nullptr; c = c->getParentComponent())
        if (auto* tc = dynamic_cast<juce::TooltipClient*> (c))
            if ((newHint = tc->getTooltip()).isNotEmpty())
                break;

    if (newHint != hint) { hint = newHint; repaint(); }
}

//==============================================================================
MainComponent::MainComponent (AppState& s)
    : state (s), toolbar (s), browser (s)
{
    menuModel.onCommand = [this] (int id) { handleCommand (id); };
    menuModel.isEnabled = [this] (int id) { return isCommandEnabled (id); };
    menuModel.isTicked  = [this] (int id) { return isCommandTicked (id); };

    addAndMakeVisible (titleRow);
    addAndMakeVisible (toolbar);
    addAndMakeVisible (browser);
    addAndMakeVisible (workspace);

    toolbar.onShowWindow = [this] (int i)
    {
        switch (i)
        {
            case 0: toggleWindow (playlist); break;
            case 1: toggleWindow (pianoRoll); break;
            case 2: toggleWindow (channelRack); break;
            case 3: toggleWindow (mixer); break;
            case 4: toggleBrowser(); break;
            case 5: showPluginManager(); break;
            default: break;
        }
    };

    browser.onClose = [this] { toggleBrowser(); };

    const char* names[] = { "Playlist", "Channel rack", "Piano roll", "Mixer" };
    for (int i = 0; i < numWindows; ++i)
    {
        windows[(size_t) i] = std::make_unique<PanelWindow> (names[i], std::make_unique<ComingSoonView> (names[i]));
        windows[(size_t) i]->onClose = [this] { menuModel.menuItemsChanged(); };
        workspace.addChildComponent (*windows[(size_t) i]);
    }

    setSize (1280, 800);
}

MainComponent::~MainComponent()
{
    menuModel.onCommand = nullptr;
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (theme::bgDeep);
}

void MainComponent::resized()
{
    auto r = getLocalBounds();
    titleRow.setBounds (r.removeFromTop (30));
    toolbar.setBounds (r.removeFromTop (52));
    browser.setVisible (browserVisible);
    if (browserVisible)
        browser.setBounds (r.removeFromLeft (260));
    workspace.setBounds (r);

    if (firstLayout && ! workspace.getBounds().isEmpty())
    {
        firstLayout = false;
        arrangeDefault();
    }
}

//==============================================================================
bool MainComponent::isCommandEnabled (int id) const
{
    switch (id)
    {
        case fileExit:
        case addRefreshPlugins: case addManagePlugins: case optManagePlugins: case addPluginPicker: case viewPluginPicker:
        case addBrowseLibrary: case addBrowseInstalled: case viewBrowserLibrary:
        case viewPlaylist: case viewChannelRack: case viewPianoRoll: case viewMixer: case viewBrowser:
        case viewCloseAll: case viewArrangeDefault: case viewArrangeRealign:
        case optMidi: case optAudio: case optGeneral: case optFile: case optProjectGeneral:
        case toolFind: case toolPanicStop:
        case helpIndex: case helpUpdates: case helpWebsite: case helpGithub: case helpAbout:
            return true;
        default:
            return false;
    }
}

bool MainComponent::isCommandTicked (int id) const
{
    auto vis = [this] (WindowId w) { return windows[(size_t) w] && windows[(size_t) w]->isVisible(); };
    switch (id)
    {
        case viewPlaylist:    return vis (playlist);
        case viewChannelRack: return vis (channelRack);
        case viewPianoRoll:   return vis (pianoRoll);
        case viewMixer:       return vis (mixer);
        case viewBrowser:     return browserVisible;
        default:              return false;
    }
}

void MainComponent::handleCommand (int id)
{
    switch (id)
    {
        case fileExit:            juce::JUCEApplication::getInstance()->systemRequestedQuit(); break;

        case addRefreshPlugins:
        case addManagePlugins:
        case optManagePlugins:
        case addPluginPicker:
        case viewPluginPicker:    showPluginManager(); break;

        case addBrowseLibrary:
        case addBrowseInstalled:
        case viewBrowserLibrary:  if (! browserVisible) toggleBrowser(); break;

        case viewPlaylist:        toggleWindow (playlist); break;
        case viewChannelRack:     toggleWindow (channelRack); break;
        case viewPianoRoll:       toggleWindow (pianoRoll); break;
        case viewMixer:           toggleWindow (mixer); break;
        case viewBrowser:         toggleBrowser(); break;
        case viewCloseAll:        closeAllWindows(); break;
        case viewArrangeDefault:
        case viewArrangeRealign:  arrangeDefault(); break;

        case optMidi:             showSettings (SettingsWindow::midi); break;
        case optAudio:            showSettings (SettingsWindow::audio); break;
        case optGeneral:          showSettings (SettingsWindow::general); break;
        case optFile:             showSettings (SettingsWindow::file); break;
        case optProjectGeneral:   showSettings (SettingsWindow::project); break;

        case toolFind:            if (! browserVisible) toggleBrowser(); browser.focusSearch(); break;
        case toolPanicStop:       toolbar.stop(); break;

        case helpIndex:
        case helpWebsite:         juce::URL ("https://yolo.su").launchInDefaultBrowser(); break;
        case helpUpdates:         juce::URL ("https://github.com/convicted3/yw-studio/releases").launchInDefaultBrowser(); break;
        case helpGithub:          juce::URL ("https://github.com/convicted3/yw-studio").launchInDefaultBrowser(); break;
        case helpAbout:           showAbout(); break;
        default: break;
    }

    menuModel.menuItemsChanged();
}

bool MainComponent::keyPressed (const juce::KeyPress& k, juce::Component*)
{
    using KP = juce::KeyPress;
    const auto mods = k.getModifiers();
    const int code = k.getKeyCode();

    auto is = [&] (int key, juce::ModifierKeys m = {}) { return code == key && mods.withoutMouseButtons() == m; };
    const juce::ModifierKeys ctrl (juce::ModifierKeys::commandModifier), alt (juce::ModifierKeys::altModifier);

    if (is (KP::F5Key))               { toggleWindow (playlist); return true; }
    if (is (KP::F6Key))               { toggleWindow (channelRack); return true; }
    if (is (KP::F7Key))               { toggleWindow (pianoRoll); return true; }
    if (is (KP::F8Key))               { showPluginManager(); return true; }
    if (is (KP::F8Key, alt))          { toggleBrowser(); return true; }
    if (is (KP::F9Key))               { toggleWindow (mixer); return true; }
    if (is (KP::F10Key))              { showSettings (SettingsWindow::midi); return true; }
    if (is (KP::F12Key))              { closeAllWindows(); return true; }
    if (is (KP::F1Key))               { handleCommand (helpIndex); return true; }
    if (is (KP::spaceKey))            { if (state.transport.playing) toolbar.stop(); else toolbar.togglePlay(); return true; }
    if (is ('L') || is ('l'))         { toolbar.togglePatSong(); return true; }
    if (is ('F', ctrl))               { handleCommand (toolFind); return true; }
    if (is ('H', ctrl))               { toolbar.stop(); return true; }

    return false;
}

//==============================================================================
void MainComponent::toggleWindow (WindowId w)
{
    auto& win = windows[(size_t) w];
    if (win->isVisible() && win.get() == workspace.getChildComponent (workspace.getNumChildComponents() - 1))
        win->setVisible (false);  // focused & visible -> hide (FL behaviour)
    else
    {
        win->setVisible (true);
        win->toFront (true);
    }
    menuModel.menuItemsChanged();
}

void MainComponent::toggleBrowser()
{
    browserVisible = ! browserVisible;
    resized();
    menuModel.menuItemsChanged();
}

void MainComponent::closeAllWindows()
{
    for (auto& w : windows) w->setVisible (false);
    menuModel.menuItemsChanged();
}

void MainComponent::arrangeDefault()
{
    auto r = workspace.getLocalBounds();
    if (r.isEmpty()) return;

    windows[playlist]->setBounds (r.reduced (6));
    windows[channelRack]->setBounds (juce::Rectangle<int> (40, 40, 640, 340).constrainedWithin (r));
    windows[pianoRoll]->setBounds (juce::Rectangle<int> (80, 80, 900, 460).constrainedWithin (r));
    windows[mixer]->setBounds (r.removeFromBottom (juce::jmin (380, r.getHeight())).reduced (6));

    windows[playlist]->setVisible (true);
    windows[channelRack]->setVisible (true);
    windows[pianoRoll]->setVisible (false);
    windows[mixer]->setVisible (false);
    windows[channelRack]->toFront (false);
    menuModel.menuItemsChanged();
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
