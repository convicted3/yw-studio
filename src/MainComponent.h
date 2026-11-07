#pragma once
#include <JuceHeader.h>
#include "AppState.h"
#include "MenuModel.h"
#include "TopBar.h"
#include "BrowserPanel.h"
#include "ChannelRackView.h"
#include "MixerView.h"
#include "PlaylistView.h"
#include "PanelWindow.h"
#include "DockWindow.h"
#include "SamplerView.h"
#include "PluginManagerWindow.h"
#include "SettingsWindow.h"

// Placeholder for the piano roll until the editor exists.
class PianoRollPlaceholder : public juce::Component
{
public:
    void paint (juce::Graphics&) override;
};

class MainComponent : public juce::Component, public juce::KeyListener, public juce::DragAndDropContainer
{
public:
    explicit MainComponent (AppState& s);
    ~MainComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    using juce::Component::keyPressed;
    bool keyPressed (const juce::KeyPress&, juce::Component*) override;

    void handleCommand (int id);

private:
    enum Panel { pBrowser, pChannelRack, pMixer, pPlaylist, pPianoRoll, pSampler, numPanels };

    bool isCommandEnabled (int id) const;
    bool isCommandTicked (int id) const;

    DockWindow& window (Panel p) const { return *windows[(size_t) p]; }
    void togglePanel (Panel p);
    void openPanel (Panel p);
    void closeAllWindows();
    void arrangeDefault();
    void saveLayout();
    bool restoreLayout();
    void showPluginManager();
    void showSettings (SettingsWindow::Tab tab);
    void showAbout();
    void refreshUi();

    AppState& state;
    MenuModel menuModel;

    TopBar topBar;
    BrowserPanel browser;
    ChannelRackView channelRack;
    MixerView mixer;
    PlaylistView playlist;
    PianoRollPlaceholder pianoRollView;
    SamplerView samplerView;

    juce::Component workspace;   // hosts all panel windows
    std::array<std::unique_ptr<DockWindow>, numPanels> windows;
    bool layoutDone = false;

    std::unique_ptr<PluginManagerWindow> pluginManager;
    std::unique_ptr<SettingsWindow> settingsWindow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
