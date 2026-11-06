#pragma once
#include <JuceHeader.h>
#include "AppState.h"
#include "MenuModel.h"
#include "ToolbarPanel.h"
#include "BrowserPanel.h"
#include "PanelWindow.h"
#include "PluginManagerWindow.h"
#include "SettingsWindow.h"

// Workspace area that hosts the floating editor windows.
class Workspace : public juce::Component
{
public:
    void paint (juce::Graphics&) override;
};

// Top row: logo, main menu and hint bar.
class TitleRow : public juce::Component, private juce::Timer
{
public:
    explicit TitleRow (juce::MenuBarModel& model);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    juce::MenuBarComponent menuBar;
    juce::Image logo;
    juce::String hint;
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
    enum WindowId { playlist = 0, channelRack, pianoRoll, mixer, numWindows };

    bool isCommandEnabled (int id) const;
    bool isCommandTicked (int id) const;

    void toggleWindow (WindowId w);
    void toggleBrowser();
    void closeAllWindows();
    void arrangeDefault();
    void showPluginManager();
    void showSettings (SettingsWindow::Tab tab);
    void showAbout();

    AppState& state;
    MenuModel menuModel;

    TitleRow titleRow { menuModel };
    ToolbarPanel toolbar;
    BrowserPanel browser;
    Workspace workspace;
    std::array<std::unique_ptr<PanelWindow>, numWindows> windows;

    bool browserVisible = true;
    bool firstLayout = true;

    std::unique_ptr<PluginManagerWindow> pluginManager;
    std::unique_ptr<SettingsWindow> settingsWindow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
