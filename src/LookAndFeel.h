#pragma once
#include <JuceHeader.h>

// YW Studio visual identity. Change the accent here to recolor the whole app.
namespace yw::theme
{
    inline const juce::Colour bgDeep      { 0xff141518 };  // workspace background
    inline const juce::Colour bg          { 0xff1d1f23 };  // panels
    inline const juce::Colour bgRaised    { 0xff272a30 };  // headers, toolbars
    inline const juce::Colour bgControl   { 0xff31353c };  // buttons, fields
    inline const juce::Colour outline     { 0xff3c4048 };
    inline const juce::Colour text        { 0xffe6e7ea };
    inline const juce::Colour textDim     { 0xff8b9099 };
    inline const juce::Colour accent      { 0xffe5484d };  // YW red
    inline const juce::Colour accentSoft  { 0xff5a2a2d };
    inline const juce::Colour lcd         { 0xff0d0e10 };  // tempo / time displays
    inline const juce::Colour lcdText     { 0xfff2f2f2 };

    inline juce::Font font (float size, bool bold = false)
    {
        return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
    }

    inline juce::Font mono (float size)
    {
        return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), size, juce::Font::bold));
    }
}

class YWLookAndFeel : public juce::LookAndFeel_V4
{
public:
    YWLookAndFeel();

    juce::Font getPopupMenuFont() override;
    juce::Font getMenuBarFont (juce::MenuBarComponent&, int, const juce::String&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                               bool isMouseOverButton, bool isButtonDown) override;

    void drawMenuBarBackground (juce::Graphics&, int width, int height, bool isMouseOverBar,
                                juce::MenuBarComponent&) override;
};
