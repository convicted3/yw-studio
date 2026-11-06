#include "DockWindow.h"
#include "LookAndFeel.h"

using namespace yw;

namespace { constexpr int snapDistance = 10; }

//==============================================================================
// Keeps the window inside the workspace and snaps its edges while resizing from the border.
class DockWindow::SnappingConstrainer : public juce::ComponentBoundsConstrainer
{
public:
    explicit SnappingConstrainer (DockWindow& w) : owner (w) {}

    void checkBounds (juce::Rectangle<int>& bounds, const juce::Rectangle<int>& previous, const juce::Rectangle<int>& limits,
                      bool top, bool left, bool bottom, bool right) override
    {
        bounds = owner.snap (bounds, false, left, top, right, bottom);
        juce::ComponentBoundsConstrainer::checkBounds (bounds, previous, limits, top, left, bottom, right);
        if (auto* p = owner.getParentComponent())
            bounds = bounds.getIntersection (p->getLocalBounds());
    }

    DockWindow& owner;
};

//==============================================================================
DockWindow::DockWindow (juce::String windowId, juce::Component& c, int caption, juce::Point<int> minSize)
    : id (std::move (windowId)), content (c), captionHeight (caption)
{
    addAndMakeVisible (content);
    // receive clicks on the panel and all its children: bring to front, and drag from the caption
    content.addMouseListener (this, true);

    constrainer = std::make_unique<SnappingConstrainer> (*this);
    constrainer->setMinimumSize (minSize.x, minSize.y);
    resizer = std::make_unique<juce::ResizableBorderComponent> (this, constrainer.get());
    resizer->setBorderThickness (juce::BorderSize<int> (border));
    addAndMakeVisible (*resizer);

    minButton.onClick   = [this] { close(); };
    maxButton.onClick   = [this] { setMaximised (! maximised); };
    closeButton.onClick = [this] { close(); };
    for (auto* b : { &minButton, &maxButton, &closeButton })
    {
        b->setIconScale (0.8f);
        addAndMakeVisible (b);
    }
}

DockWindow::~DockWindow()
{
    content.removeMouseListener (this);
}

void DockWindow::open()
{
    setVisible (true);
    toFront (false);
    if (onVisibilityChanged) onVisibilityChanged();
}

void DockWindow::close()
{
    setVisible (false);
    if (onVisibilityChanged) onVisibilityChanged();
}

bool DockWindow::isFrontmost() const
{
    if (auto* p = getParentComponent())
        for (int i = p->getNumChildComponents(); --i >= 0;)
            if (auto* c = p->getChildComponent (i); c != nullptr && c->isVisible() && dynamic_cast<DockWindow*> (c) != nullptr)
                return c == this;
    return false;
}

void DockWindow::toggle()
{
    if (isVisible() && isFrontmost()) close();
    else open();
}

void DockWindow::setMaximised (bool shouldBe)
{
    if (shouldBe == maximised) return;
    if (shouldBe)
    {
        restoreBounds = getBounds();
        maximised = true;
        if (auto* p = getParentComponent()) setBounds (p->getLocalBounds());
    }
    else
    {
        maximised = false;
        setBounds (restoreBounds);
    }
    maxButton.setTooltip (maximised ? "Restore" : "Maximize");
    resizer->setVisible (! maximised);
    toFront (false);
}

void DockWindow::setNormalBounds (juce::Rectangle<int> b)
{
    if (maximised) restoreBounds = b;
    else setBounds (b);
}

//==============================================================================
void DockWindow::paint (juce::Graphics& g)
{
    // window frame
    g.fillAll (juce::Colour (0xff15191c));
}

void DockWindow::paintOverChildren (juce::Graphics& g)
{
    const bool front = isFrontmost();
    g.setColour (front ? juce::Colour (0xff59646a) : juce::Colour (0xff2c3438));
    g.drawRect (getLocalBounds(), 1);
}

void DockWindow::resized()
{
    const int b = maximised ? 1 : border;
    auto inner = getLocalBounds().reduced (b);
    content.setBounds (inner);
    resizer->setBounds (getLocalBounds());

    auto caption = inner.removeFromTop (captionHeight).reduced (1, 1);
    closeButton.setBounds (caption.removeFromRight (21));
    maxButton.setBounds (caption.removeFromRight (21));
    minButton.setBounds (caption.removeFromRight (21));
    for (auto* btn : { &minButton, &maxButton, &closeButton }) btn->toFront (false);
}

void DockWindow::parentSizeChanged()
{
    if (auto* p = getParentComponent())
    {
        if (maximised) setBounds (p->getLocalBounds());
        else if (! p->getLocalBounds().isEmpty())
        {
            auto b = getBounds();
            b.setSize (juce::jmin (b.getWidth(), p->getWidth()), juce::jmin (b.getHeight(), p->getHeight()));
            setBounds (b.constrainedWithin (p->getLocalBounds()));
        }
    }
}

//==============================================================================
juce::Rectangle<int> DockWindow::snap (juce::Rectangle<int> b, bool moving, bool left, bool top, bool right, bool bottom) const
{
    auto* p = getParentComponent();
    if (p == nullptr || juce::ModifierKeys::getCurrentModifiers().isAltDown()) return b; // Alt = no snapping

    juce::Array<int> xs { 0, p->getWidth() }, ys { 0, p->getHeight() };
    if (getSnapTargets)
        for (auto& r : getSnapTargets())
        {
            // snap to the other window's edges, and to the other side of a 1px gap
            xs.addArray ({ r.getX(), r.getRight(), r.getX() - 1, r.getRight() + 1 });
            ys.addArray ({ r.getY(), r.getBottom(), r.getY() - 1, r.getBottom() + 1 });
        }

    auto nearest = [] (int v, const juce::Array<int>& cands) -> std::optional<int>
    {
        std::optional<int> best;
        int bestD = snapDistance + 1;
        for (auto c : cands) if (std::abs (c - v) < bestD) { bestD = std::abs (c - v); best = c; }
        return best;
    };

    if (moving)
    {
        // move the whole window so its nearest edge lands on a target
        auto dx = [&]() -> int
        {
            auto l = nearest (b.getX(), xs), r = nearest (b.getRight(), xs);
            if (l && (! r || std::abs (*l - b.getX()) <= std::abs (*r - b.getRight()))) return *l - b.getX();
            if (r) return *r - b.getRight();
            return 0;
        }();
        auto dy = [&]() -> int
        {
            auto t = nearest (b.getY(), ys), bo = nearest (b.getBottom(), ys);
            if (t && (! bo || std::abs (*t - b.getY()) <= std::abs (*bo - b.getBottom()))) return *t - b.getY();
            if (bo) return *bo - b.getBottom();
            return 0;
        }();
        return b.translated (dx, dy);
    }

    if (left)   if (auto v = nearest (b.getX(), xs))      b.setLeft (*v);
    if (right)  if (auto v = nearest (b.getRight(), xs))  b.setRight (*v);
    if (top)    if (auto v = nearest (b.getY(), ys))      b.setTop (*v);
    if (bottom) if (auto v = nearest (b.getBottom(), ys)) b.setBottom (*v);
    return b;
}

//==============================================================================
void DockWindow::mouseDown (const juce::MouseEvent& e)
{
    toFront (false);
    if (auto* p = getParentComponent()) p->repaint(); // frame highlight of the others

    dragging = false;
    // only the panel's own background in the caption row starts a drag (not its buttons)
    if (e.eventComponent == &content && e.getEventRelativeTo (&content).y < captionHeight && ! e.mods.isPopupMenu())
    {
        dragging = true;
        dragStartBounds = getBounds();
    }
}

void DockWindow::mouseDrag (const juce::MouseEvent& e)
{
    if (! dragging) return;

    if (maximised)
    {
        // dragging a maximised window restores it under the mouse, like a desktop window
        const float rel = (float) e.getMouseDownX() / (float) juce::jmax (1, getWidth());
        setMaximised (false);
        dragStartBounds = getBounds().withX (getX() + e.getMouseDownX() - juce::roundToInt (rel * getWidth())).withY (0);
        setBounds (dragStartBounds);
    }

    auto* p = getParentComponent();
    auto b = dragStartBounds.translated (e.getDistanceFromDragStartX(), e.getDistanceFromDragStartY());
    b = snap (b, true, false, false, false, false);
    if (p != nullptr)
        b = b.constrainedWithin (p->getLocalBounds());
    setBounds (b);
}

void DockWindow::mouseUp (const juce::MouseEvent&) { dragging = false; }

void DockWindow::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (e.eventComponent == &content && e.getEventRelativeTo (&content).y < captionHeight)
        setMaximised (! maximised);
}
