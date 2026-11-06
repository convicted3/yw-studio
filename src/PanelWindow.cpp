#include "PanelWindow.h"
#include "LookAndFeel.h"

using namespace yw;

PanelWindow::PanelWindow (const juce::String& t, std::unique_ptr<juce::Component> c)
    : title (t)
{
    setContent (std::move (c));

    closeButton.setTooltip ("Close window");
    closeButton.onClick = [this] { setVisible (false); if (onClose) onClose(); };
    addAndMakeVisible (closeButton);

    constrainer.setMinimumSize (240, 120);
    constrainer.setMinimumOnscreenAmounts (headerHeight, 60, 40, 60);
    corner = std::make_unique<juce::ResizableCornerComponent> (this, &constrainer);
    addAndMakeVisible (*corner);
}

void PanelWindow::setContent (std::unique_ptr<juce::Component> c)
{
    if (content) removeChildComponent (content.get());
    content = std::move (c);
    if (content) addAndMakeVisible (*content);
    if (corner) corner->toFront (false);
    resized();
}

void PanelWindow::paint (juce::Graphics& g)
{
    auto r = getLocalBounds();
    g.fillAll (theme::panelBody);

    auto header = r.removeFromTop (headerHeight);
    g.setColour (theme::panelHeader);
    g.fillRect (header);

    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawHorizontalLine (header.getBottom() - 1, 0.0f, (float) getWidth());

    g.setColour (theme::text);
    g.setFont (theme::font (18.0f, theme::Weight::medium));
    g.drawText (title, header.reduced (10, 0), juce::Justification::centredLeft);

    g.setColour (theme::outline);
    g.drawRect (getLocalBounds(), 1);
}

void PanelWindow::resized()
{
    auto r = getLocalBounds();
    auto header = r.removeFromTop (headerHeight);
    closeButton.setBounds (header.removeFromRight (headerHeight).reduced (3));

    if (content) content->setBounds (r.reduced (1, 0).withTrimmedBottom (1));
    if (corner)  corner->setBounds (getWidth() - 14, getHeight() - 14, 14, 14);
}

void PanelWindow::mouseDown (const juce::MouseEvent& e)
{
    toFront (true);
    if (e.y < headerHeight)
        dragger.startDraggingComponent (this, e);
}

void PanelWindow::mouseDrag (const juce::MouseEvent& e)
{
    if (e.getMouseDownY() < headerHeight)
        dragger.dragComponent (this, e, &constrainer);
}

void PanelWindow::mouseDoubleClick (const juce::MouseEvent& e)
{
    // Double-click on the header maximises inside the workspace / restores.
    if (e.y >= headerHeight || getParentComponent() == nullptr) return;

    auto full = getParentComponent()->getLocalBounds();
    if (getBounds() == full && ! restoreBounds.isEmpty())
        setBounds (restoreBounds);
    else
    {
        restoreBounds = getBounds();
        setBounds (full);
    }
}

void PanelWindow::parentSizeChanged()
{
    if (auto* p = getParentComponent())
        setBounds (getBounds().constrainedWithin (p->getLocalBounds()));
}

void ComingSoonView::paint (juce::Graphics& g)
{
    g.fillAll (theme::workspace);

    // light grid to hint at an editor surface
    g.setColour (theme::panelBody);
    for (int x = 0; x < getWidth(); x += 24) g.drawVerticalLine (x, 0.0f, (float) getHeight());
    for (int y = 0; y < getHeight(); y += 24) g.drawHorizontalLine (y, 0.0f, (float) getWidth());

    g.setColour (theme::textDim);
    g.setFont (theme::font (17.0f));
    g.drawText (name + juce::String::fromUTF8 (" \xe2\x80\x94 in development"), getLocalBounds(), juce::Justification::centred);
}
