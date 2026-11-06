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

private:
    AppState& state;
    ToolButton menuButton { icons::Id::arrowRight, "Mixer options", ToolButton::Style::flat };
    juce::OwnedArray<ToolButton> headerIcons;
    juce::ComboBox layout;
    ToolButton closeButton { icons::Id::close, "Close", ToolButton::Style::flat };

    juce::Viewport viewport;
    juce::Component stripHolder;
    juce::OwnedArray<Strip> strips;
    int selected = 0;
};
