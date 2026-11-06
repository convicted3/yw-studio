#pragma once
#include <JuceHeader.h>

// Startup animation: transparent window, the Y drops in from above and the W rises from below,
// both stop hard with a short bounce and lock into the logo, then "yolo.su" fades in underneath.
class SplashWindow : public juce::Component
{
public:
    SplashWindow();
    ~SplashWindow() override;

    void show();

    // Plays the exit (fade) once the intro has finished, then calls onDone.
    void finish (std::function<void()> onDone);

    void paint (juce::Graphics&) override;

    // Renders one frame at a given time (ms since start). Used by paint() and the frame dump tool.
    static void renderFrame (juce::Graphics&, juce::Rectangle<float> area, double ms, double exitMs = -1.0);

    static constexpr double introLengthMs = 1250.0;
    static constexpr double exitLengthMs = 240.0;

private:
    double elapsedMs() const { return juce::Time::getMillisecondCounterHiRes() - startMs; }
    void tick();

    double startMs = 0.0;
    double exitStartMs = -1.0;
    std::function<void()> doneCallback;
    std::unique_ptr<juce::VBlankAttachment> vblank;
};
