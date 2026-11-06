#include "SplashWindow.h"
#include "LookAndFeel.h"
#include "AppState.h"

using namespace yw;

SplashWindow::SplashWindow()
{
    logo = juce::ImageCache::getFromMemory (BinaryData::yw_logo_png, BinaryData::yw_logo_pngSize);
    setOpaque (true);
    setSize (560, 320);
}

void SplashWindow::show()
{
    addToDesktop (juce::ComponentPeer::windowHasDropShadow);
    centreWithSize (getWidth(), getHeight());
    setAlwaysOnTop (true);
    setVisible (true);
    toFront (false);
}

void SplashWindow::setStatus (const juce::String& s)
{
    status = s;
    repaint();
}

void SplashWindow::paint (juce::Graphics& g)
{
    auto r = getLocalBounds();

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff050506), 0.0f, 0.0f,
                                             juce::Colour (0xff1a1b1f), 0.0f, (float) getHeight(), false));
    g.fillAll();

    // Logo
    if (logo.isValid())
    {
        auto area = r.withTrimmedBottom (110).reduced (0, 40).toFloat();
        g.drawImage (logo, area, juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize);
    }

    // Product name + version
    auto text = r.removeFromBottom (110);
    g.setColour (theme::text);
    g.setFont (theme::font (24.0f, true));
    g.drawText ("YW STUDIO", text.removeFromTop (34), juce::Justification::centred);

    g.setColour (theme::textDim);
    g.setFont (theme::font (13.0f));
    g.drawText (juce::String ("Version ") + AppState::version + "  \xc2\xb7  yolo.su",
                text.removeFromTop (20), juce::Justification::centred);

    // Status line
    auto bottom = text.reduced (24, 10);
    g.setFont (theme::font (12.0f));
    g.drawText (status, bottom.removeFromTop (18), juce::Justification::centredLeft);

    // Progress bar
    auto bar = bottom.removeFromTop (3).toFloat();
    g.setColour (theme::bgControl);
    g.fillRect (bar);
    g.setColour (theme::accent);
    g.fillRect (bar.withWidth (bar.getWidth() * juce::jlimit (0.0f, 1.0f, progress)));

    g.setColour (theme::outline);
    g.drawRect (getLocalBounds(), 1);
}
