#pragma once
#include <JuceHeader.h>

// YW Studio palette and fonts. Layout and tone follow the classic pattern-DAW workspace;
// all graphics are drawn in code.
namespace yw::theme
{
    // Shell
    inline const juce::Colour topBar      { 0xff2d3437 };
    inline const juce::Colour hintBg      { 0xff23292b };
    inline const juce::Colour panelHeader { 0xff272e30 };
    inline const juce::Colour panelBody   { 0xff31363a };
    inline const juce::Colour workspace   { 0xff1b2124 };
    inline const juce::Colour splitter    { 0xff1a2123 };

    // Browser
    inline const juce::Colour browserBg   { 0xff060c0f };
    inline const juce::Colour browserBg2  { 0xff0f171b };
    inline const juce::Colour browserTabs { 0xff161c20 };
    inline const juce::Colour tagsBar     { 0xff12171a };

    // Channel rack
    inline const juce::Colour rackRow     { 0xff171c20 };
    inline const juce::Colour stepA       { 0xff434a4f };
    inline const juce::Colour stepB       { 0xff5a3b3e };
    inline const juce::Colour stepOn      { 0xffe9ecee };

    // Mixer
    inline const juce::Colour mixerBg     { 0xff20262a };
    inline const juce::Colour mixerStrip  { 0xff232a2e };
    inline const juce::Colour meterBg     { 0xff15191c };

    // Playlist
    inline const juce::Colour pickerBg    { 0xff1d242a };
    inline const juce::Colour trackHeader { 0xff272d31 };
    inline const juce::Colour ruler       { 0xff070f13 };
    inline const juce::Colour gridA       { 0xff1e282e };
    inline const juce::Colour gridB       { 0xff182228 };
    inline const juce::Colour gridLine    { 0xff0c161c };
    inline const juce::Colour gridBeat    { 0xff142026 };

    // Controls
    inline const juce::Colour face        { 0xff3a4145 };
    inline const juce::Colour faceDark    { 0xff2a3034 };
    inline const juce::Colour faceActive  { 0xffd6dadb };
    inline const juce::Colour outline     { 0xff15191b };
    inline const juce::Colour lcd         { 0xff1a2022 };

    // Text
    inline const juce::Colour text        { 0xffc3c8cb };
    inline const juce::Colour textDim     { 0xff8e9192 };
    inline const juce::Colour textBright  { 0xfff2f2f0 };
    inline const juce::Colour lcdText     { 0xffeae6d8 };

    // Brand
    inline const juce::Colour accent      { 0xffe5484d };  // YW red
    inline const juce::Colour accentSoft  { 0xff4a2a2d };
    inline const juce::Colour green       { 0xff8fd36a };

    enum class Weight { regular, medium, semibold };
    juce::Font font (float height, Weight w = Weight::regular);
    inline juce::Font bold (float height) { return font (height, Weight::semibold); }
}

class YWLookAndFeel : public juce::LookAndFeel_V4
{
public:
    YWLookAndFeel();

    juce::Typeface::Ptr getTypefaceForFont (const juce::Font&) override;

    juce::Font getPopupMenuFont() override;
    juce::Font getMenuBarFont (juce::MenuBarComponent&, int, const juce::String&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getLabelFont (juce::Label&) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawMenuBarBackground (juce::Graphics&, int, int, bool, juce::MenuBarComponent&) override;
    void drawMenuBarItem (juce::Graphics&, int width, int height, int itemIndex, const juce::String& itemText,
                          bool isMouseOverItem, bool isMenuOpen, bool isMouseOverBar, juce::MenuBarComponent&) override;
    int getMenuBarItemWidth (juce::MenuBarComponent&, int itemIndex, const juce::String& itemText) override;
    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown, int buttonX, int buttonY,
                       int buttonW, int buttonH, juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    void drawPopupMenuBackground (juce::Graphics&, int width, int height) override;
    void drawScrollbar (juce::Graphics&, juce::ScrollBar&, int x, int y, int width, int height, bool isScrollbarVertical,
                        int thumbStartPosition, int thumbSize, bool isMouseOver, bool isMouseDown) override;
    int getDefaultScrollbarWidth() override { return 12; }
};
