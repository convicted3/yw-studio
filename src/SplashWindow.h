#pragma once
#include <JuceHeader.h>

// Startup splash: logo, version and a status line showing what is loading.
class SplashWindow : public juce::Component
{
public:
    SplashWindow();

    void show();
    void setStatus (const juce::String& s);

    void paint (juce::Graphics&) override;

private:
    juce::Image logo;
    juce::String status { "Starting..." };
    float progress = 0.0f;

public:
    void setProgress (float p) { progress = p; repaint(); }
};
