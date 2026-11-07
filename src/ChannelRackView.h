#pragma once
#include <JuceHeader.h>
#include "AppState.h"
#include "Widgets.h"

// Data of one channel (shared with the channel's sampler window).
struct ChannelInfo
{
    juce::String name { "Sampler" };
    juce::File sample;                  // empty = no sample loaded
    float volume = 0.78f, pan = 0.5f, pitch = 0.5f;
    bool reverse = false, loop = false;
    int mixerTrack = 1;

    using Ptr = std::shared_ptr<ChannelInfo>;
};

// Channel rack: header controls + one row per channel with mute, pan, volume, mixer track,
// name button, selector and the step sequencer. Accepts samples dragged from the browser or Explorer:
// dropped on a channel it replaces that channel's sample, dropped below the channels it adds new ones.
class ChannelRackView : public juce::Component,
                        public juce::DragAndDropTarget,
                        public juce::FileDragAndDropTarget
{
public:
    explicit ChannelRackView (AppState&);
    ~ChannelRackView() override;

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;

    // Samples
    void addSampleChannels (const juce::Array<juce::File>& files, int insertIndex = -1);
    void sendToSelected (const juce::File& file);
    void refreshRows();

    std::function<void (ChannelInfo::Ptr)> onOpenChannel;   // name button clicked
    std::function<void (ChannelInfo::Ptr)> onChannelChanged; // sample / name changed from the rack
    std::function<void()> onClose;

    // Drag and drop (browser items and Explorer files)
    bool isInterestedInDragSource (const SourceDetails&) override;
    void itemDragMove (const SourceDetails&) override;
    void itemDragExit (const SourceDetails&) override;
    void itemDropped (const SourceDetails&) override;
    bool isInterestedInFileDrag (const juce::StringArray&) override;
    void fileDragMove (const juce::StringArray&, int, int) override;
    void fileDragExit (const juce::StringArray&) override;
    void filesDropped (const juce::StringArray&, int, int) override;

    static constexpr int headerHeight = 24;
    static constexpr int rowHeight = 30;

    class ChannelRow;

private:
    void addChannel (ChannelInfo::Ptr info, int index = -1);
    void removeChannel (int index);
    void cloneChannel (int index);
    void loadSampleInto (int index);
    void renameChannel (int index);
    int rowIndexAt (juce::Point<int> p) const;  // -1 = below the rows
    void updateDropTarget (juce::Point<int> p, bool active);
    void dropFiles (const juce::Array<juce::File>& files, juce::Point<int> p);
    static juce::Array<juce::File> audioFilesFrom (const juce::StringArray& paths);

    AppState& state;
    ToolButton menuButton  { icons::Id::arrowRight, "Channel rack options", ToolButton::Style::flat };
    ToolButton undoButton  { icons::Id::undo, "Undo (Ctrl+Z)", ToolButton::Style::flat };
    juce::ComboBox filter;
    Knob swing { "Global swing", 0.0f };
    HintArea lengthBox;
    ToolButton graphButton { icons::Id::graph, "Graph editor (Ctrl+K)", ToolButton::Style::flat };
    ToolButton stepsButton { icons::Id::steps, "Step sequencer view", ToolButton::Style::flat };

    juce::Viewport viewport;
    juce::Component rowHolder;
    juce::OwnedArray<ChannelRow> rows;
    ToolButton addButton { icons::Id::plus, "Add a channel (or drop samples here)", ToolButton::Style::flat };

    bool dropActive = false;
    int dropRow = -1;            // row to replace, or -1 = add new channel(s)
    std::unique_ptr<juce::FileChooser> chooser;
};
