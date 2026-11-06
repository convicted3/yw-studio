#pragma once
#include <JuceHeader.h>
#include "AppState.h"

// Options > Settings: MIDI, Audio, General, File, Project tabs.
class SettingsWindow : public juce::DocumentWindow
{
public:
    enum Tab { midi = 0, audio, general, file, project };

    explicit SettingsWindow (AppState& s);
    void showTab (Tab t);
    void closeButtonPressed() override;

private:
    AppState& state;
    juce::TabbedComponent* tabs = nullptr;
};
