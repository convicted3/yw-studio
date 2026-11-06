#include "LookAndFeel.h"

using namespace yw;

YWLookAndFeel::YWLookAndFeel()
    : juce::LookAndFeel_V4 (juce::LookAndFeel_V4::ColourScheme (
          theme::bg,          // windowBackground
          theme::bgControl,   // widgetBackground
          theme::bgRaised,    // menuBackground
          theme::outline,     // outline
          theme::text,        // defaultText
          theme::bgControl,   // defaultFill
          theme::text,        // highlightedText
          theme::accent,      // highlightedFill
          theme::text))       // menuText
{
    setColour (juce::ResizableWindow::backgroundColourId, theme::bg);
    setColour (juce::DocumentWindow::textColourId, theme::text);

    setColour (juce::PopupMenu::backgroundColourId, theme::bgRaised);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, theme::accent);
    setColour (juce::PopupMenu::textColourId, theme::text);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);

    setColour (juce::TextButton::buttonColourId, theme::bgControl);
    setColour (juce::TextButton::buttonOnColourId, theme::accent);
    setColour (juce::TextButton::textColourOffId, theme::text);
    setColour (juce::TextButton::textColourOnId, juce::Colours::white);

    setColour (juce::ComboBox::backgroundColourId, theme::bgControl);
    setColour (juce::ComboBox::outlineColourId, theme::outline);
    setColour (juce::ComboBox::textColourId, theme::text);
    setColour (juce::ComboBox::arrowColourId, theme::textDim);

    setColour (juce::TextEditor::backgroundColourId, theme::bgDeep);
    setColour (juce::TextEditor::outlineColourId, theme::outline);
    setColour (juce::TextEditor::focusedOutlineColourId, theme::accent);
    setColour (juce::TextEditor::textColourId, theme::text);

    setColour (juce::Label::textColourId, theme::text);

    setColour (juce::Slider::thumbColourId, theme::text);
    setColour (juce::Slider::rotarySliderFillColourId, theme::accent);
    setColour (juce::Slider::rotarySliderOutlineColourId, theme::bgDeep);
    setColour (juce::Slider::trackColourId, theme::accent);
    setColour (juce::Slider::backgroundColourId, theme::bgDeep);

    setColour (juce::TreeView::backgroundColourId, theme::bg);
    setColour (juce::TreeView::linesColourId, theme::outline);
    setColour (juce::TreeView::selectedItemBackgroundColourId, theme::accentSoft);

    setColour (juce::ListBox::backgroundColourId, theme::bg);
    setColour (juce::ListBox::textColourId, theme::text);

    setColour (juce::TabbedComponent::backgroundColourId, theme::bg);
    setColour (juce::TabbedButtonBar::tabOutlineColourId, theme::outline);
    setColour (juce::TabbedButtonBar::frontOutlineColourId, theme::accent);

    setColour (juce::ScrollBar::thumbColourId, theme::bgControl);
    setColour (juce::ToggleButton::tickColourId, theme::accent);
    setColour (juce::ToggleButton::textColourId, theme::text);
    setColour (juce::TooltipWindow::backgroundColourId, theme::bgRaised);
    setColour (juce::TooltipWindow::textColourId, theme::text);
}

juce::Font YWLookAndFeel::getPopupMenuFont()                                         { return theme::font (14.0f); }
juce::Font YWLookAndFeel::getMenuBarFont (juce::MenuBarComponent&, int, const juce::String&) { return theme::font (14.0f); }
juce::Font YWLookAndFeel::getTextButtonFont (juce::TextButton&, int h)               { return theme::font (juce::jmin (13.0f, h * 0.6f), true); }

void YWLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour& base,
                                          bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    auto c = base;
    if (down)      c = c.brighter (0.15f);
    else if (over) c = c.brighter (0.07f);

    g.setColour (c);
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (theme::outline);
    g.drawRoundedRectangle (r, 3.0f, 1.0f);
}

void YWLookAndFeel::drawMenuBarBackground (juce::Graphics& g, int, int, bool, juce::MenuBarComponent&)
{
    g.fillAll (theme::bgRaised);
}
