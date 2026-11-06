#pragma once
#include <JuceHeader.h>

// A floating window inside the workspace (Playlist, Channel Rack, Piano Roll, Mixer...).
// Draggable by its header, resizable from the corner, kept inside the workspace.
class PanelWindow : public juce::Component
{
public:
    PanelWindow (const juce::String& title, std::unique_ptr<juce::Component> content);

    void setContent (std::unique_ptr<juce::Component> c);
    juce::Component* getContent() const { return content.get(); }

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void parentSizeChanged() override;

    std::function<void()> onClose;

    static constexpr int headerHeight = 24;

private:
    juce::String title;
    std::unique_ptr<juce::Component> content;
    juce::TextButton closeButton { juce::String::fromUTF8 ("\xc3\x97") };
    juce::ComponentDragger dragger;
    juce::ComponentBoundsConstrainer constrainer;
    std::unique_ptr<juce::ResizableCornerComponent> corner;
    juce::Rectangle<int> restoreBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PanelWindow)
};

// Placeholder content for windows that are not implemented yet.
class ComingSoonView : public juce::Component
{
public:
    explicit ComingSoonView (juce::String what) : name (std::move (what)) {}
    void paint (juce::Graphics&) override;
private:
    juce::String name;
};
