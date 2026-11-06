#pragma once
#include <JuceHeader.h>
#include "AppState.h"
#include "Widgets.h"

// Left-docked browser: header, icon tabs, content tree, tags bar.
class BrowserPanel : public juce::Component, private juce::ChangeListener
{
public:
    explicit BrowserPanel (AppState& s);
    ~BrowserPanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void rebuildTree();
    void showPluginDatabase();
    void focusSearch();

    std::function<void()> onClose;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override { rebuildTree(); }
    void setTab (int index);
    juce::File userDataFolder() const;

    AppState& state;
    int currentTab = 0;

    ToolButton menuButton    { icons::Id::arrowRight, "Browser options", ToolButton::Style::flat };
    ToolButton upButton      { icons::Id::up, "Collapse all", ToolButton::Style::flat };
    ToolButton backButton    { icons::Id::undo, "Refresh (Ctrl+R)", ToolButton::Style::flat };
    juce::OwnedArray<ToolButton> tabs;
    ToolButton tagsFolder    { icons::Id::folder, "Add folder to browser", ToolButton::Style::flat };
    ToolButton tagsSearch    { icons::Id::search, "Search (Ctrl+F)", ToolButton::Style::flat };
    juce::TextEditor search;
    juce::TreeView tree;
    std::unique_ptr<juce::TreeViewItem> root;
    bool searchVisible = false;
};
