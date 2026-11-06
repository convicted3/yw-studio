#pragma once
#include <JuceHeader.h>
#include "AppState.h"
#include "Widgets.h"

// Playlist: tool bar, clip picker, track headers, bar ruler and the arrangement grid.
class PlaylistView : public juce::Component, private juce::ScrollBar::Listener
{
public:
    explicit PlaylistView (AppState&);
    ~PlaylistView() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    std::function<void()> onClose;

    class Picker;
    class TrackList;
    class Grid;

private:
    void scrollBarMoved (juce::ScrollBar*, double newRangeStart) override;

    AppState& state;
    ToolButton menuButton  { icons::Id::arrowRight, "Playlist options", ToolButton::Style::flat };
    ToolButton snapButton  { icons::Id::magnet, "Snap", ToolButton::Style::flat };
    juce::OwnedArray<ToolButton> tools;
    ToolButton closeButton { icons::Id::close, "Close", ToolButton::Style::flat };
    ToolButton minButton   { icons::Id::minimise, "Minimize", ToolButton::Style::flat };

    std::unique_ptr<Picker> picker;
    std::unique_ptr<TrackList> trackList;
    std::unique_ptr<Grid> grid;
    juce::ScrollBar vScroll { true }, hScroll { false };
    int currentTool = 0;
};
