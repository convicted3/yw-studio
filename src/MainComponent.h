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
#include "PluginManagerWindow.h"
#include "SettingsWindow.h"

// Thin draggable divider between docked panels.
class Splitter : public juce::Component
{
public:
    explicit Splitter (bool vertical) : isVertical (vertical)
    {
        setMouseCursor (vertical ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::UpDownResizeCursor);
    }
    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent&) override { if (onStart) onStart(); }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (onDrag) onDrag (isVertical ? e.getDistanceFromDragStartX() : e.getDistanceFromDragStartY());
    }
    std::function<void()> onStart;
    std::function<void (int)> onDrag;
private:
    bool isVertical;
};

class MainComponent : public juce::Component, public juce::KeyListener
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
    enum Panel { pBrowser, pChannelRack, pMixer, pPlaylist, numPanels };

    bool isCommandEnabled (int id) const;
    bool isCommandTicked (int id) const;

    void togglePanel (Panel p);
    void toggleFloating (PanelWindow& w);
    void closeAllWindows();
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
    Splitter browserSplit { true }, middleSplit { true }, rackSplit { false };

    juce::Component floatingLayer;   // hosts floating windows (piano roll ...)
    std::unique_ptr<PanelWindow> pianoRoll;

    std::array<bool, numPanels> visible { true, true, true, true };
    int browserW = 325, middleW = 500, rackH = 360;
    int dragStartValue = 0;

    std::unique_ptr<PluginManagerWindow> pluginManager;
    std::unique_ptr<SettingsWindow> settingsWindow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
