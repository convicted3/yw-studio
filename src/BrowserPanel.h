#pragma once
#include <JuceHeader.h>
#include "AppState.h"
#include "Widgets.h"

// Left-docked browser: header, icon tabs, content tree, tags bar.
class BrowserPanel : public juce::Component, public juce::FileDragAndDropTarget, private juce::ChangeListener
{
public:
    explicit BrowserPanel (AppState& s);
    ~BrowserPanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void rebuildTree();
    void showPluginDatabase();
    void focusSearch();

    // Callbacks the tree items use to talk back to the browser
    struct Actions
    {
        std::function<void (const juce::File&)> openInNewChannel, sendToSelected, removeFolder;
        std::function<bool (const juce::File&)> isUserFolder;
    };

    std::function<void()> onClose;
    std::function<void (const juce::File&)> onOpenInNewChannel, onSendToSelected;

    void addFolders (const juce::StringArray& paths);
    void removeFolder (const juce::File&);
    void chooseFolderToAdd();

    bool isInterestedInFileDrag (const juce::StringArray&) override;
    void fileDragEnter (const juce::StringArray&, int, int) override;
    void fileDragExit (const juce::StringArray&) override;
    void filesDropped (const juce::StringArray&, int, int) override;
    void paintOverChildren (juce::Graphics&) override;

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
    bool searchVisible = false, dropHighlight = false;
    std::unique_ptr<juce::FileChooser> chooser;
    std::unique_ptr<Actions> actions;
};
