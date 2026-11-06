#pragma once
#include <JuceHeader.h>
#include "Widgets.h"

// A window inside the workspace that hosts one panel (browser, channel rack, mixer, playlist, piano roll).
// The panel's own header bar is the caption: drag its empty area to move, double-click to maximise.
// Resizable from every edge and corner, minimise / maximise / close buttons on the right of the caption,
// snaps to the workspace edges and to the other windows while moving or resizing.
class DockWindow : public juce::Component
{
public:
    DockWindow (juce::String id, juce::Component& content, int captionHeight, juce::Point<int> minSize);
    ~DockWindow() override;

    const juce::String& getId() const { return id; }

    void open();                      // show and bring to front
    void close();                     // hide
    void toggle();                    // hide if focused and visible, else open
    bool isFrontmost() const;

    void setMaximised (bool);
    bool isMaximised() const { return maximised; }

    // Normal (non-maximised) bounds, used for saving the layout
    juce::Rectangle<int> getNormalBounds() const { return maximised ? restoreBounds : getBounds(); }
    void setNormalBounds (juce::Rectangle<int>);

    std::function<void()> onVisibilityChanged;
    std::function<juce::Array<juce::Rectangle<int>>()> getSnapTargets; // other windows' bounds

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;
    void parentSizeChanged() override;

    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

    static constexpr int border = 3;

private:
    class SnappingConstrainer;
    juce::Rectangle<int> snap (juce::Rectangle<int> b, bool moving, bool left, bool top, bool right, bool bottom) const;

    juce::String id;
    juce::Component& content;
    int captionHeight;

    ToolButton minButton   { icons::Id::minimise, "Minimize", ToolButton::Style::flat };
    ToolButton maxButton   { icons::Id::maximise, "Maximize", ToolButton::Style::flat };
    ToolButton closeButton { icons::Id::close,    "Close",    ToolButton::Style::flat };

    std::unique_ptr<SnappingConstrainer> constrainer;
    std::unique_ptr<juce::ResizableBorderComponent> resizer;

    bool maximised = false, dragging = false;
    juce::Rectangle<int> restoreBounds, dragStartBounds;
};
