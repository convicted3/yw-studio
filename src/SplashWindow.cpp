#include "SplashWindow.h"
#include "LookAndFeel.h"
#include "Icons.h"

using namespace yw;

namespace
{
    juce::Path polygon (std::initializer_list<juce::Point<float>> pts)
    {
        juce::Path p;
        bool first = true;
        for (auto& pt : pts) { if (first) p.startNewSubPath (pt); else p.lineTo (pt); first = false; }
        p.closeSubPath();
        return p;
    }

    // The logo split along x = 111 into the Y and the W (logo space: 304 x 192).
    const juce::Path& letterY()
    {
        static const juce::Path p = polygon ({ { 111.8f, 77.9f }, { 89.2f, 119.0f }, { 89.5f, 164.0f }, { 53.0f, 179.3f },
                                              { 52.5f, 126.0f }, { 7.0f, 3.5f }, { 41.6f, 15.0f }, { 71.0f, 94.6f },
                                              { 97.6f, 30.0f }, { 111.8f, 31.6f } });
        return p;
    }

    const juce::Path& letterW()
    {
        static const juce::Path p = polygon ({ { 261.0f, 183.4f }, { 220.5f, 165.0f }, { 198.0f, 75.9f }, { 174.0f, 154.9f },
                                              { 135.4f, 154.0f }, { 110.2f, 77.9f }, { 110.2f, 31.4f }, { 134.0f, 34.2f },
                                              { 155.0f, 122.2f }, { 179.5f, 34.0f }, { 216.0f, 27.0f }, { 240.0f, 135.7f },
                                              { 262.6f, 14.0f }, { 296.8f, 3.0f } });
        return p;
    }

    float easeOutCubic (float x) { x = juce::jlimit (0.0f, 1.0f, x); return 1.0f - std::pow (1.0f - x, 3.0f); }

    // Motion of one letter. offset: remaining distance in its travel direction (px, negative = overshoot).
    struct Motion
    {
        float offset = 0.0f;
        float stretch = 0.0f;  // > 0 elongated along travel, < 0 squashed
        float speed = 0.0f;    // 0..1, drives the motion trail
        bool visible = false;
        double sinceImpact = -1.0; // seconds
    };

    constexpr double fallMs = 165.0;

    Motion motion (double ms, double delayMs, float travel)
    {
        Motion m;
        const double t = ms - delayMs;
        if (t < 0.0) return m;
        m.visible = true;

        if (t < fallMs)
        {
            // accelerating slam
            const float u = (float) (t / fallMs);
            m.offset  = travel * (1.0f - std::pow (u, 2.6f));
            m.speed   = std::pow (u, 1.6f);
            m.stretch = 0.22f * m.speed;
            return m;
        }

        // hard stop with a short damped bounce
        const double tau = (t - fallMs) / 1000.0;
        const double w = juce::MathConstants<double>::twoPi * 7.5;
        m.sinceImpact = tau;
        m.offset  = (float) (-travel * 0.075 * std::exp (-15.0 * tau) * std::sin (w * tau));
        m.stretch = (float) (-0.16 * std::exp (-22.0 * tau) * std::cos (w * tau));
        m.speed   = 0.0f;
        return m;
    }

    struct LetterShape
    {
        juce::Path main;
        std::vector<std::pair<juce::Path, float>> ghosts; // path, alpha
        bool visible = false;
    };

    LetterShape buildLetter (const juce::Path& letter, const juce::AffineTransform& logoToScreen, const Motion& m, bool fromTop)
    {
        LetterShape out;
        if (! m.visible) return out;
        out.visible = true;

        // squash & stretch around the leading edge (bottom of the Y, top of the W)
        const auto b = letter.getBounds();
        const float anchorY = fromTop ? b.getBottom() : b.getY();
        const float sy = 1.0f + m.stretch, sx = 1.0f - m.stretch * 0.45f;
        const float dir = fromTop ? -1.0f : 1.0f;
        const auto shape = juce::AffineTransform::scale (sx, sy, b.getCentreX(), anchorY);

        auto place = [&] (float extra) { return shape.followedBy (logoToScreen).translated (0.0f, dir * (m.offset + extra)); };

        if (m.speed > 0.02f)
            for (int i = 4; i >= 1; --i)
            {
                juce::Path ghost (letter);
                ghost.applyTransform (place (m.speed * 26.0f * (float) i));
                out.ghosts.push_back ({ ghost, 0.16f * m.speed * (1.0f - i / 5.0f) });
            }

        out.main = letter;
        out.main.applyTransform (place (0.0f));
        return out;
    }

    void drawTracked (juce::Graphics& g, const juce::String& text, juce::Font f, float tracking, juce::Point<float> centre, juce::Colour c)
    {
        // manual letter spacing so it can animate
        float total = 0.0f;
        juce::Array<float> widths;
        for (auto ch : text)
        {
            const float w = juce::GlyphArrangement::getStringWidth (f, juce::String::charToString (ch));
            widths.add (w);
            total += w;
        }
        total += tracking * (float) (text.length() - 1);

        float x = centre.x - total * 0.5f;
        g.setFont (f);
        g.setColour (c);
        for (int i = 0; i < text.length(); ++i)
        {
            g.drawSingleLineText (juce::String::charToString (text[i]), juce::roundToInt (x), juce::roundToInt (centre.y));
            x += widths[i] + tracking;
        }
    }
}

//==============================================================================
SplashWindow::SplashWindow()
{
    setOpaque (false);
    setSize (560, 420);
}

SplashWindow::~SplashWindow() = default;

void SplashWindow::show()
{
    addToDesktop (0); // no frame, no shadow: only the logo is visible
    centreWithSize (getWidth(), getHeight());
    setAlwaysOnTop (true);
    setVisible (true);
    toFront (false);

    startMs = juce::Time::getMillisecondCounterHiRes();
    vblank = std::make_unique<juce::VBlankAttachment> (this, [this] { tick(); });
}

void SplashWindow::finish (std::function<void()> onDone)
{
    doneCallback = std::move (onDone);
    // start the exit after the intro has fully played
    exitStartMs = juce::jmax (elapsedMs(), introLengthMs);
}

void SplashWindow::tick()
{
    repaint();
    if (exitStartMs >= 0.0 && elapsedMs() >= exitStartMs + exitLengthMs && doneCallback)
    {
        auto cb = std::move (doneCallback);
        doneCallback = nullptr;
        vblank.reset();
        cb();
    }
}

void SplashWindow::paint (juce::Graphics& g)
{
    const double ms = elapsedMs();
    renderFrame (g, getLocalBounds().toFloat(), ms, exitStartMs >= 0.0 && ms >= exitStartMs ? ms - exitStartMs : -1.0);
}

void SplashWindow::renderFrame (juce::Graphics& g, juce::Rectangle<float> area, double ms, double exitMs)
{
    float alpha = 1.0f, scale = 1.0f;
    if (exitMs >= 0.0)
    {
        const float e = (float) juce::jlimit (0.0, 1.0, exitMs / exitLengthMs);
        alpha = 1.0f - e * e;
        scale = 1.0f + 0.035f * easeOutCubic (e);
    }
    if (alpha <= 0.0f) return;

    const float logoW = 300.0f, s = logoW / icons::logoWidth;
    const float logoH = icons::logoHeight * s;
    const auto c = area.getCentre();

    // camera shake right after the W lands
    const auto mY = motion (ms, 40.0, 300.0f);
    const auto mW = motion (ms, 95.0, 300.0f);
    float shakeX = 0.0f, shakeY = 0.0f;
    if (mW.sinceImpact >= 0.0)
    {
        const double t = mW.sinceImpact;
        shakeX = (float) (3.5 * std::exp (-28.0 * t) * std::sin (t * 95.0));
        shakeY = (float) (2.0 * std::exp (-28.0 * t) * std::cos (t * 120.0));
    }

    auto logoToScreen = juce::AffineTransform::scale (s)
                            .translated (c.x - logoW * 0.5f + shakeX, c.y - logoH * 0.5f - 26.0f + shakeY)
                            .followedBy (juce::AffineTransform::scale (scale, scale, c.x, c.y));

    juce::Graphics::ScopedSaveState state (g);
    g.reduceClipRegion (area.toNearestInt());

    const auto y = buildLetter (letterY(), logoToScreen, mY, true);
    const auto w = buildLetter (letterW(), logoToScreen, mW, false);

    // once both have settled, draw the original single shape (perfectly seamless)
    const bool settled = mY.sinceImpact > 0.35 && mW.sinceImpact > 0.35;
    juce::Path whole;
    if (settled) { whole = icons::logoPath(); whole.applyTransform (logoToScreen); }

    for (auto* l : { &y, &w })
        for (auto& [ghost, a] : l->ghosts)
        {
            g.setColour (juce::Colours::white.withAlpha (a * alpha));
            g.fillPath (ghost);
        }

    const juce::DropShadow shadow (juce::Colours::black.withAlpha (0.55f * alpha), 22, { 0, 6 });
    if (settled) shadow.drawForPath (g, whole);
    else for (auto* l : { &y, &w }) if (l->visible) shadow.drawForPath (g, l->main);

    g.setColour (juce::Colours::white.withAlpha (alpha));
    if (settled) g.fillPath (whole);
    else for (auto* l : { &y, &w }) if (l->visible) g.fillPath (l->main);

    // impact glow once both letters are in
    if (mW.sinceImpact >= 0.0)
    {
        const float glow = (float) (std::exp (-7.0 * mW.sinceImpact)) * alpha;
        if (glow > 0.01f)
        {
            juce::Path full (icons::logoPath());
            full.applyTransform (logoToScreen);
            for (int i = 3; i >= 1; --i)
            {
                g.setColour (juce::Colours::white.withAlpha (glow * 0.09f));
                g.strokePath (full, juce::PathStrokeType ((float) i * 6.0f, juce::PathStrokeType::curved));
            }
        }
    }

    // yolo.su
    const double textStart = 500.0, textLen = 460.0;
    if (ms > textStart)
    {
        const float e = easeOutCubic ((float) ((ms - textStart) / textLen));
        const auto f = theme::font (36.0f, theme::Weight::medium);
        const float tracking = juce::jmap (e, 26.0f, 5.0f);
        const float ty = c.y + logoH * 0.5f + 44.0f + (1.0f - e) * 18.0f;
        juce::Point<float> pos (c.x + shakeX * 0.3f, ty);
        pos = pos.transformedBy (juce::AffineTransform::scale (scale, scale, c.x, c.y));

        drawTracked (g, "yolo.su", f, tracking, pos.translated (0.0f, 2.0f), juce::Colours::black.withAlpha (0.35f * e * alpha));
        drawTracked (g, "yolo.su", f, tracking, pos, juce::Colours::white.withAlpha (0.92f * e * alpha));
    }
}
