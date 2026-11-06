#pragma once
#include <JuceHeader.h>
#include "AppState.h"

// Options > Manage plugins: search paths, scanning and the list of installed plugins.
class PluginManagerWindow : public juce::DocumentWindow
{
public:
    explicit PluginManagerWindow (AppState& s);
    void closeButtonPressed() override { setVisible (false); }

private:
    class Content;
};
