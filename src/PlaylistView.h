#pragma once
#include <JuceHeader.h>
#include "AppState.h"
#include "Widgets.h"

// Playlist: tool bar, clip picker, track headers, bar ruler and the arrangement grid.
// Zoom with the mouse wheel over the ruler (or Ctrl+wheel on the grid), resize tracks by dragging
// the bottom edge of a track header, double-click the "..." to collapse a track, click to restore.
class PlaylistView : public juce::Component, private juce::ScrollBar::Listener, private juce::Timer
{
public:
    explicit PlaylistView (AppState&);
    ~PlaylistView() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    std::function<void()> onClose;

    struct Clip { int track; double start, length; int pattern; juce::File audio = {}; }; // audio set = audio clip

    struct Model
    {
        struct Track
        {
            float height = defaultHeight, target = defaultHeight, restore = defaultHeight;
            bool collapsed = false, muted = false;
        };

        static constexpr float defaultHeight = 56.0f, minHeight = 30.0f, maxHeight = 240.0f, collapsedHeight = 18.0f;
        static constexpr double minBarWidth = 10.0, maxBarWidth = 1600.0;

        std::vector<Track> tracks;
        std::vector<Clip> clips;

        double barWidth = 48.0, targetBarWidth = 48.0;  // pixels per bar (animated towards target)
        double scrollX = 0.0;                           // pixels
        double zoomAnchorX = 0.0;                       // view x kept fixed while zooming
        float scrollY = 0.0f;

        float yOf (int track) const;                    // content y of a track's top
        float totalHeight() const;
        int trackAt (float contentY) const;             // -1 if none
    };

    class Picker;
    class TrackList;
    class Grid;
    class TimelineBar;

private:
    void scrollBarMoved (juce::ScrollBar*, double newRangeStart) override;
    void timerCallback() override;
    void updateScrollBars();
    void zoomAt (double viewX, int wheelSteps);
    void scrollVertically (float deltaY);

    AppState& state;
    Model model;

    ToolButton menuButton  { icons::Id::arrowRight, "Playlist options", ToolButton::Style::flat };
    ToolButton snapButton  { icons::Id::magnet, "Snap", ToolButton::Style::flat };
    juce::OwnedArray<ToolButton> tools;
    ToolButton closeButton { icons::Id::close, "Close", ToolButton::Style::flat };
    ToolButton minButton   { icons::Id::minimise, "Minimize", ToolButton::Style::flat };

    std::unique_ptr<Picker> picker;
    std::unique_ptr<TrackList> trackList;
    std::unique_ptr<Grid> grid;
    juce::ScrollBar vScroll { true };
    std::unique_ptr<TimelineBar> timeline;

    double songLengthBars() const;
    void scrollToX (double x);
    int currentTool = 0;
    bool updatingScrollBars = false;
    double zoomAnchorBar = 0.0;
};
