#pragma once
#include <JuceHeader.h>
#include "AppState.h"
#include "ChannelRackView.h"
#include "Widgets.h"

// Channel settings window for a sampler channel: header with pan / volume / pitch / mixer track,
// tabs, and the sample page with the waveform. Drop a sample on it to replace the channel's sample.
class SamplerView : public juce::Component,
                    public juce::DragAndDropTarget,
                    public juce::FileDragAndDropTarget,
                    private juce::Timer
{
public:
    explicit SamplerView (AppState&);

    void setChannel (ChannelInfo::Ptr);
    ChannelInfo::Ptr getChannel() const { return info; }

    std::function<void (ChannelInfo::Ptr)> onChanged;

    void paint (juce::Graphics&) override;
    void resized() override;

    bool isInterestedInDragSource (const SourceDetails&) override;
    void itemDragEnter (const SourceDetails&) override { dropping = true; repaint(); }
    void itemDragExit (const SourceDetails&) override  { dropping = false; repaint(); }
    void itemDropped (const SourceDetails&) override;
    bool isInterestedInFileDrag (const juce::StringArray&) override;
    void fileDragEnter (const juce::StringArray&, int, int) override { dropping = true; repaint(); }
    void fileDragExit (const juce::StringArray&) override { dropping = false; repaint(); }
    void filesDropped (const juce::StringArray&, int, int) override;

    static constexpr int headerHeight = 24;

private:
    void timerCallback() override;
    void setSample (const juce::File&);
    void chooseSample();

    AppState& state;
    ChannelInfo::Ptr info;

    ToolButton menuButton { icons::Id::arrowRight, "Channel options", ToolButton::Style::flat };
    ToolButton prevPreset { icons::Id::arrowRight, "Previous sample in folder", ToolButton::Style::flat };
    ToolButton nextPreset { icons::Id::arrowRight, "Next sample in folder", ToolButton::Style::flat };
    Knob pan    { "Panning", 0.5f, true };
    Knob volume { "Volume", 0.78f };
    Knob pitch  { "Pitch", 0.5f, true };
    juce::OwnedArray<juce::TextButton> tabs;
    juce::TextButton openButton { "Open sample..." };
    juce::ToggleButton reverse { "Reverse" }, loop { "Loop" };
    juce::Rectangle<int> waveArea, infoArea;
    bool dropping = false;
    std::unique_ptr<juce::FileChooser> chooser;
};
