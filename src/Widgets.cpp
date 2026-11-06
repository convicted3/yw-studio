#include "Widgets.h"
#include "LookAndFeel.h"

using namespace yw;

ToolButton::ToolButton (icons::Id i, const juce::String& hint, Style s)
    : juce::Button (hint), icon (i), style (s)
{
    setTooltip (hint);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void ToolButton::paintButton (juce::Graphics& g, bool over, bool down)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    const bool on = getToggleState();

    if (style == Style::raised || on)
    {
        juce::Colour face = on ? theme::faceActive : theme::face;
        if (down) face = face.brighter (0.1f); else if (over) face = face.brighter (0.05f);

        g.setGradientFill (juce::ColourGradient (face.brighter (0.06f), 0, r.getY(), face.darker (0.1f), 0, r.getBottom(), false));
        g.fillRoundedRectangle (r, 2.5f);
        g.setColour (theme::outline);
        g.drawRoundedRectangle (r, 2.5f, 1.0f);
        if (! on)
        {
            g.setColour (juce::Colours::white.withAlpha (0.05f));
            g.drawHorizontalLine (1, r.getX() + 2, r.getRight() - 2);
        }
    }
    else if (over)
    {
        g.setColour (juce::Colours::white.withAlpha (down ? 0.12f : 0.06f));
        g.fillRoundedRectangle (r, 2.5f);
    }

    auto ink = hasIconColour ? iconColour : (on ? juce::Colour (0xff1e2326) : theme::text);
    if (! isEnabled()) ink = ink.withAlpha (0.35f);
    else if (over && ! on) ink = ink.brighter (0.25f);

    const float side = juce::jmin (r.getWidth(), r.getHeight()) * iconScale;
    icons::draw (g, icon, r.withSizeKeepingCentre (side, side), ink);
}

//==============================================================================
Knob::Knob (juce::String n, float def, bool bi)
    : name (std::move (n)), value (def), defaultValue (def), bipolar (bi)
{
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
}

void Knob::setValue (float v, bool notify)
{
    v = juce::jlimit (0.0f, 1.0f, v);
    if (v == value) return;
    value = v;
    repaint();
    if (notify && onChange) onChange (value);
}

void Knob::paint (juce::Graphics& g)
{
    const auto side = (float) juce::jmin (getWidth(), getHeight());
    auto r = getLocalBounds().toFloat().withSizeKeepingCentre (side, side).reduced (1.0f);
    const auto c = r.getCentre();

    // shadow ring
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillEllipse (r.translated (0.0f, 1.0f));

    // body
    juce::ColourGradient body (juce::Colour (largeStyle ? 0xff4b5357 : 0xff464e52), c.x, r.getY(),
                               juce::Colour (0xff1c2124), c.x, r.getBottom(), false);
    g.setGradientFill (body);
    g.fillEllipse (r);
    g.setColour (theme::outline);
    g.drawEllipse (r, 1.0f);

    // inner cap
    auto cap = r.reduced (r.getWidth() * 0.16f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3d4448), c.x, cap.getY(), juce::Colour (0xff22282b), c.x, cap.getBottom(), false));
    g.fillEllipse (cap);

    // value arc for bipolar knobs (pan) when off-centre
    const float start = -juce::MathConstants<float>::pi * 0.75f, end = juce::MathConstants<float>::pi * 0.75f;
    const float angle = start + value * (end - start);
    if (bipolar && std::abs (value - 0.5f) > 0.01f)
    {
        juce::Path arc;
        arc.addCentredArc (c.x, c.y, r.getWidth() * 0.5f - 1.0f, r.getHeight() * 0.5f - 1.0f, 0.0f,
                           0.0f, angle, true);
        g.setColour (theme::accent.withAlpha (0.9f));
        g.strokePath (arc, juce::PathStrokeType (1.6f));
    }

    // indicator
    const float len = r.getWidth() * 0.36f;
    juce::Point<float> tip (c.x + std::sin (angle) * len, c.y - std::cos (angle) * len);
    g.setColour (theme::textBright);
    g.drawLine ({ c.getPointOnCircumference (len * 0.25f, angle), tip }, juce::jmax (1.4f, side * 0.08f));
}

void Knob::mouseDown (const juce::MouseEvent&) { dragStart = value; }

void Knob::mouseDrag (const juce::MouseEvent& e)
{
    const float sens = e.mods.isShiftDown() ? 1000.0f : 160.0f;
    setValue (dragStart - (float) e.getDistanceFromDragStartY() / sens);
}

void Knob::mouseDoubleClick (const juce::MouseEvent&) { setValue (defaultValue); }

void Knob::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    setValue (value + (w.deltaY > 0 ? 1.0f : -1.0f) * (e.mods.isShiftDown() ? 0.005f : 0.03f));
}

juce::String Knob::getTooltip()
{
    juce::String v;
    if (display) v = display (value);
    else if (bipolar)
    {
        const int pct = juce::roundToInt ((value - 0.5f) * 200.0f);
        v = pct == 0 ? "Centered" : juce::String (std::abs (pct)) + "% " + (pct < 0 ? "left" : "right");
    }
    else v = juce::String (juce::roundToInt (value * 100.0f)) + "%";
    return name + " - " + v;
}

//==============================================================================
void drawPanelHeader (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title, icons::Id titleIcon)
{
    g.setColour (theme::panelHeader);
    g.fillRect (r);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawHorizontalLine (r.getBottom() - 1, (float) r.getX(), (float) r.getRight());

    if (title.isNotEmpty())
    {
        auto t = r;
        if (titleIcon != icons::Id::none)
        {
            icons::draw (g, titleIcon, t.removeFromLeft (20).toFloat().reduced (2.0f), theme::text);
            t.removeFromLeft (4);
        }
        g.setColour (theme::text);
        g.setFont (theme::font (18.0f, theme::Weight::medium));
        g.drawText (title, t, juce::Justification::centredLeft, true);
    }
}
