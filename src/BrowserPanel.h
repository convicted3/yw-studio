#pragma once
#include <JuceHeader.h>
#include "AppState.h"

// Left-docked browser: tabs, search and a tree of project content, folders and the plugin library.
class BrowserPanel : public juce::Component, private juce::ChangeListener
{
public:
    explicit BrowserPanel (AppState& s);
    ~BrowserPanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void rebuildTree();
    void focusSearch() { search.grabKeyboardFocus(); }

    std::function<void()> onClose;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override { rebuildTree(); }
    void setTab (int index);

    AppState& state;
    int currentTab = 0;

    juce::TextButton menuButton { juce::String::fromUTF8 ("\xe2\x96\xbe") };
    juce::TextButton collapseButton { "-" }, refreshButton { juce::String::fromUTF8 ("\xe2\x9f\xb3") }, closeButton { juce::String::fromUTF8 ("\xc3\x97") };
    juce::OwnedArray<juce::TextButton> tabs;
    juce::TextEditor search;
    juce::TreeView tree;
    std::unique_ptr<juce::TreeViewItem> root;
};
