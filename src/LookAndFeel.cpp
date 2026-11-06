#include "LookAndFeel.h"

using namespace yw;

namespace
{
    struct Fonts
    {
        juce::Typeface::Ptr regular  = juce::Typeface::createSystemTypefaceFor (BinaryData::BarlowSemiCondensedRegular_ttf,  BinaryData::BarlowSemiCondensedRegular_ttfSize);
        juce::Typeface::Ptr medium   = juce::Typeface::createSystemTypefaceFor (BinaryData::BarlowSemiCondensedMedium_ttf,   BinaryData::BarlowSemiCondensedMedium_ttfSize);
        juce::Typeface::Ptr semibold = juce::Typeface::createSystemTypefaceFor (BinaryData::BarlowSemiCondensedSemiBold_ttf, BinaryData::BarlowSemiCondensedSemiBold_ttfSize);

        static Fonts& get() { static Fonts f; return f; }
    };
}

juce::Font theme::font (float height, Weight w)
{
    auto& f = Fonts::get();
    auto tf = w == Weight::semibold ? f.semibold : w == Weight::medium ? f.medium : f.regular;
    return juce::Font (juce::FontOptions (tf).withHeight (height));
}

YWLookAndFeel::YWLookAndFeel()
    : juce::LookAndFeel_V4 (juce::LookAndFeel_V4::ColourScheme (
          theme::panelBody, theme::face, theme::panelHeader, theme::outline, theme::text,
          theme::face, theme::textBright, theme::accent, theme::text))
{
    using namespace theme;
    setColour (juce::ResizableWindow::backgroundColourId, panelBody);
    setColour (juce::DocumentWindow::textColourId, text);

    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff2a3034));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff4d565b));
    setColour (juce::PopupMenu::textColourId, text);
    setColour (juce::PopupMenu::highlightedTextColourId, textBright);
    setColour (juce::PopupMenu::headerTextColourId, textDim);

    setColour (juce::TextButton::buttonColourId, face);
    setColour (juce::TextButton::buttonOnColourId, faceActive);
    setColour (juce::TextButton::textColourOffId, text);
    setColour (juce::TextButton::textColourOnId, juce::Colour (0xff1c2124));

    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff1e2427));
    setColour (juce::ComboBox::outlineColourId, outline);
    setColour (juce::ComboBox::textColourId, text);
    setColour (juce::ComboBox::arrowColourId, textDim);

    setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff11161a));
    setColour (juce::TextEditor::outlineColourId, outline);
    setColour (juce::TextEditor::focusedOutlineColourId, juce::Colour (0xff4d565b));
    setColour (juce::TextEditor::textColourId, text);
    setColour (juce::CaretComponent::caretColourId, textBright);

    setColour (juce::Label::textColourId, text);
    setColour (juce::Slider::thumbColourId, text);
    setColour (juce::Slider::trackColourId, accent);
    setColour (juce::Slider::backgroundColourId, juce::Colour (0xff11161a));
    setColour (juce::Slider::rotarySliderFillColourId, accent);
    setColour (juce::Slider::rotarySliderOutlineColourId, outline);

    setColour (juce::TreeView::backgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::TreeView::linesColourId, juce::Colours::transparentBlack);
    setColour (juce::TreeView::selectedItemBackgroundColourId, juce::Colour (0xff27333a));

    setColour (juce::ListBox::backgroundColourId, panelBody);
    setColour (juce::ListBox::textColourId, text);
    setColour (juce::TabbedComponent::backgroundColourId, panelBody);
    setColour (juce::TabbedButtonBar::tabOutlineColourId, outline);
    setColour (juce::TabbedButtonBar::frontOutlineColourId, accent);
    setColour (juce::ToggleButton::tickColourId, accent);
    setColour (juce::ToggleButton::textColourId, text);
    setColour (juce::ScrollBar::thumbColourId, juce::Colour (0xff3a4347));
    setColour (juce::AlertWindow::backgroundColourId, juce::Colour (0xff2a3034));
    setColour (juce::AlertWindow::textColourId, text);
    setColour (juce::AlertWindow::outlineColourId, outline);
}

juce::Typeface::Ptr YWLookAndFeel::getTypefaceForFont (const juce::Font& f)
{
    if (f.getTypefaceName() == juce::Font::getDefaultSansSerifFontName()
        || f.getTypefaceName() == juce::Font::getDefaultMonospacedFontName())
    {
        auto& fonts = Fonts::get();
        return f.isBold() ? fonts.semibold : fonts.regular;
    }
    return juce::LookAndFeel_V4::getTypefaceForFont (f);
}

juce::Font YWLookAndFeel::getPopupMenuFont()                                          { return theme::font (15.0f); }
juce::Font YWLookAndFeel::getMenuBarFont (juce::MenuBarComponent&, int, const juce::String&) { return theme::font (17.5f, theme::Weight::medium).withHorizontalScale (0.84f); }
juce::Font YWLookAndFeel::getTextButtonFont (juce::TextButton&, int h)                { return theme::font (juce::jmin (15.0f, h * 0.62f), theme::Weight::medium); }
juce::Font YWLookAndFeel::getComboBoxFont (juce::ComboBox&)                           { return theme::font (15.0f); }
juce::Font YWLookAndFeel::getLabelFont (juce::Label&)                                 { return theme::font (15.0f); }

void YWLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour& base, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    auto c = base;
    if (down) c = c.brighter (0.12f); else if (over) c = c.brighter (0.06f);
    g.setGradientFill (juce::ColourGradient (c.brighter (0.05f), 0, r.getY(), c.darker (0.08f), 0, r.getBottom(), false));
    g.fillRoundedRectangle (r, 2.5f);
    g.setColour (theme::outline);
    g.drawRoundedRectangle (r, 2.5f, 1.0f);
}

void YWLookAndFeel::drawMenuBarBackground (juce::Graphics&, int, int, bool, juce::MenuBarComponent&) {}

int YWLookAndFeel::getMenuBarItemWidth (juce::MenuBarComponent& m, int index, const juce::String& text)
{
    juce::GlyphArrangement ga;
    ga.addLineOfText (getMenuBarFont (m, index, text), text, 0.0f, 0.0f);
    return juce::roundToInt (ga.getBoundingBox (0, -1, true).getWidth()) + 7;
}

void YWLookAndFeel::drawMenuBarItem (juce::Graphics& g, int w, int h, int, const juce::String& text,
                                     bool over, bool open, bool, juce::MenuBarComponent& bar)
{
    if (open || over)
    {
        g.setColour (juce::Colour (0xff434c51));
        g.fillRect (0, 2, w, h - 4);
    }
    g.setColour (open || over ? theme::textBright : theme::text);
    g.setFont (getMenuBarFont (bar, 0, text));
    g.drawFittedText (text, 0, 0, w, h, juce::Justification::centred, 1);
}

void YWLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (r, 2.0f);
    g.setColour (theme::outline);
    g.drawRoundedRectangle (r, 2.0f, 1.0f);

    // small right-pointing arrow like the toolbar selectors
    juce::Path p;
    const float ax = (float) w - 11.0f, ay = h * 0.5f;
    p.addTriangle (ax - 2.5f, ay - 4.0f, ax - 2.5f, ay + 4.0f, ax + 2.5f, ay);
    g.setColour (theme::textDim);
    g.fillPath (p);
}

void YWLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (4, 0, box.getWidth() - 20, box.getHeight());
    label.setFont (getComboBoxFont (box));
}

void YWLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    g.fillAll (findColour (juce::PopupMenu::backgroundColourId));
    g.setColour (theme::outline);
    g.drawRect (0, 0, w, h);
}

void YWLookAndFeel::drawScrollbar (juce::Graphics& g, juce::ScrollBar&, int x, int y, int w, int h, bool vertical,
                                   int thumbStart, int thumbSize, bool over, bool down)
{
    g.setColour (juce::Colour (0xff1a2023));
    g.fillRect (x, y, w, h);
    auto thumb = vertical ? juce::Rectangle<int> (x + 2, thumbStart, w - 4, thumbSize)
                          : juce::Rectangle<int> (thumbStart, y + 2, thumbSize, h - 4);
    g.setColour (juce::Colour (down ? 0xff56626a : over ? 0xff4a555c : 0xff3a4347));
    g.fillRoundedRectangle (thumb.toFloat(), 2.0f);
}
