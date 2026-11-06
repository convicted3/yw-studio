#pragma once
#include <JuceHeader.h>
#include "AppState.h"
#include "Widgets.h"

// Mixer: header, dB scale strip, master and insert tracks with meters, pan, stereo, fader and routing.
class MixerView : public juce::Component
{
public:
    explicit MixerView (AppState&);
    ~MixerView() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    std::function<void()> onClose;

    class Strip;
    class Inspector;

    // Per-track effect slots (UI state; audio processing comes with the engine)
    struct TrackFx
    {
        std::array<juce::String, 10> names;
        std::array<float, 10> mix;
        std::array<bool, 10> enabled;
        TrackFx() { mix.fill (1.0f); enabled.fill (true); }
    };

private:
    AppState& state;
    ToolButton menuButton { icons::Id::arrowRight, "Mixer options", ToolButton::Style::flat };
    juce::OwnedArray<ToolButton> headerIcons;
    juce::ComboBox layout;
    ToolButton closeButton { icons::Id::close, "Close", ToolButton::Style::flat };

    juce::Viewport viewport;
    juce::Component stripHolder;
    juce::OwnedArray<Strip> strips;
    std::vector<TrackFx> fx;
    std::unique_ptr<Inspector> inspector;
    int selected = 1;

    void selectStrip (int index);
};
