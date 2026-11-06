#pragma once
#include <JuceHeader.h>

// Maximise for the frameless main window: fills the monitor's work area (keeps the taskbar visible),
// instead of true full screen which would cover the taskbar.
namespace yw::window
{
    inline bool isMaximised (const juce::Component& top)
    {
        return (bool) top.getProperties().getWithDefault ("ywMaximised", false);
    }

    inline void setMaximised (juce::Component& top, bool shouldBeMaximised)
    {
        auto& props = top.getProperties();
        if (shouldBeMaximised == isMaximised (top)) return;

        if (shouldBeMaximised)
        {
            props.set ("ywRestore", top.getBounds().toString());
            props.set ("ywMaximised", true);
            if (auto* display = juce::Desktop::getInstance().getDisplays().getDisplayForRect (top.getScreenBounds()))
                top.setBounds (display->userArea);
        }
        else
        {
            props.set ("ywMaximised", false);
            auto restore = juce::Rectangle<int>::fromString (props.getWithDefault ("ywRestore", "").toString());
            if (restore.isEmpty() || restore.getWidth() < 400)
                if (auto* display = juce::Desktop::getInstance().getDisplays().getDisplayForRect (top.getScreenBounds()))
                    restore = display->userArea.reduced (display->userArea.getWidth() / 10, display->userArea.getHeight() / 10);
            top.setBounds (restore);
        }
    }

    inline void toggleMaximised (juce::Component& top) { setMaximised (top, ! isMaximised (top)); }
}
