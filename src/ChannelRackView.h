#pragma once
#include <JuceHeader.h>
#include "AppState.h"
#include "Widgets.h"

// Channel rack: header controls + one row per channel with mute, pan, volume, mixer track,
// name button, selector and the step sequencer.
class ChannelRackView : public juce::Component
{
public:
    explicit ChannelRackView (AppState&);
    ~ChannelRackView() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    std::function<void()> onClose;

    static constexpr int headerHeight = 24;
    static constexpr int rowHeight = 30;

    class ChannelRow;

private:
    void addChannel (const juce::String& name);

    AppState& state;
    ToolButton menuButton  { icons::Id::arrowRight, "Channel rack options", ToolButton::Style::flat };
    ToolButton undoButton  { icons::Id::undo, "Undo (Ctrl+Z)", ToolButton::Style::flat };
    juce::ComboBox filter;
    Knob swing { "Global swing", 0.0f };
    HintArea lengthBox;
    ToolButton graphButton { icons::Id::graph, "Graph editor (Ctrl+K)", ToolButton::Style::flat };
    ToolButton stepsButton { icons::Id::steps, "Step sequencer view", ToolButton::Style::flat };
    ToolButton closeButton { icons::Id::close, "Close", ToolButton::Style::flat };

    juce::OwnedArray<ChannelRow> rows;
    ToolButton addButton { icons::Id::plus, "Add a plugin", ToolButton::Style::flat };
};
