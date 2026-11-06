#pragma once
#include <JuceHeader.h>

// Startup animation: transparent window, the Y drops in from above and the W rises from below,
// both stop hard with a short bounce and lock into the logo, then "yolo.su" fades in underneath.
class SplashWindow : public juce::Component, private juce::Timer
{
public:
    SplashWindow();
    ~SplashWindow() override;

    // Shows the window and plays the intro; onIntroDone is called (asynchronously) once the
    // letters have landed, so heavy startup work can run while the logo holds still.
    void show (std::function<void()> onIntroDone);

    // Plays the exit (fade), then calls onDone asynchronously (safe to delete the splash there).
    void finish (std::function<void()> onDone);

    void paint (juce::Graphics&) override;

    // Renders one frame at a given time (ms since start). Used by paint() and the frame dump tool.
    static void renderFrame (juce::Graphics&, juce::Rectangle<float> area, double ms, double exitMs = -1.0);

    static constexpr double introLengthMs = 1250.0;
    static constexpr double exitLengthMs = 240.0;

private:
    void timerCallback() override;

    // Animation clock advances by at most ~one frame per tick, so a stall on the message
    // thread pauses the animation instead of skipping it.
    double animMs = 0.0, lastTickMs = 0.0;
    double exitStartMs = -1.0;
    std::function<void()> introCallback, doneCallback;
};
