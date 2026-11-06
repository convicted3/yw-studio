#pragma once
#include <JuceHeader.h>
#include "Icons.h"

// Square tool button with a vector icon. Toggle state shows the light "active" face.
class ToolButton : public juce::Button
{
public:
    enum class Style { raised, flat };

    ToolButton (icons::Id icon, const juce::String& hint, Style s = Style::raised);
    void paintButton (juce::Graphics&, bool over, bool down) override;

    void setIcon (icons::Id i) { icon = i; repaint(); }
    void setIconColour (juce::Colour c) { iconColour = c; hasIconColour = true; repaint(); }
    void setIconScale (float s) { iconScale = s; repaint(); }

private:
    icons::Id icon;
    Style style;
    juce::Colour iconColour;
    bool hasIconColour = false;
    float iconScale = 0.72f;
};

// Small rotary knob. Drag vertically (Shift = fine), wheel, double-click to reset.
class Knob : public juce::Component, public juce::TooltipClient
{
public:
    Knob (juce::String name, float defaultValue, bool bipolar = false);

    void setValue (float v, bool notify = true);
    float getValue() const { return value; }

    void setStyle (bool large) { largeStyle = large; repaint(); }
    void setDisplay (std::function<juce::String (float)> f) { display = std::move (f); }

    std::function<void (float)> onChange;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    juce::String getTooltip() override;

private:
    juce::String name;
    float value, defaultValue, dragStart = 0.0f;
    bool bipolar, largeStyle = false;
    std::function<juce::String (float)> display;
};

// Simple label-like component that shows a hint in the hint bar.
class HintArea : public juce::Component, public juce::SettableTooltipClient {};

// Draws a panel caption bar: background, optional title with icon.
void drawPanelHeader (juce::Graphics&, juce::Rectangle<int> r, const juce::String& title, icons::Id titleIcon);
