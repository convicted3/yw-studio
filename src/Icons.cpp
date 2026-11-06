#include "Icons.h"

namespace icons
{
namespace
{
    // Helper that maps unit coordinates (0..1) into the target rectangle.
    struct U
    {
        juce::Rectangle<float> r;
        float x (float u) const { return r.getX() + u * r.getWidth(); }
        float y (float v) const { return r.getY() + v * r.getHeight(); }
        juce::Point<float> p (float u, float v) const { return { x (u), y (v) }; }
        juce::Rectangle<float> rect (float u, float v, float w, float h) const
        { return { x (u), y (v), w * r.getWidth(), h * r.getHeight() }; }
        float s (float k) const { return k * r.getWidth(); }
    };

    void line (juce::Graphics& g, const U& u, float x1, float y1, float x2, float y2, float t)
    {
        g.drawLine (u.x (x1), u.y (y1), u.x (x2), u.y (y2), t);
    }

    void tri (juce::Graphics& g, const U& u, float x1, float y1, float x2, float y2, float x3, float y3)
    {
        juce::Path p;
        p.addTriangle (u.p (x1, y1), u.p (x2, y2), u.p (x3, y3));
        g.fillPath (p);
    }

    void stroke (juce::Graphics& g, const juce::Path& p, float t)
    {
        g.strokePath (p, juce::PathStrokeType (t, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
}

const juce::Path& logoPath()
{
    static const juce::Path path = []
    {
        static const float pts[][2] = {
            { 261.0f, 183.4f }, { 220.5f, 165.0f }, { 198.0f, 75.9f }, { 174.0f, 154.9f }, { 135.4f, 154.0f },
            { 111.0f, 77.9f },  { 89.2f, 119.0f },  { 89.5f, 164.0f }, { 53.0f, 179.3f },  { 52.5f, 126.0f },
            { 7.0f, 3.5f },     { 41.6f, 15.0f },   { 71.0f, 94.6f },  { 97.6f, 30.0f },   { 134.0f, 34.2f },
            { 155.0f, 122.2f }, { 179.5f, 34.0f },  { 216.0f, 27.0f }, { 240.0f, 135.7f }, { 262.6f, 14.0f },
            { 296.8f, 3.0f } };
        juce::Path p;
        p.startNewSubPath (pts[0][0], pts[0][1]);
        for (size_t i = 1; i < std::size (pts); ++i)
            p.lineTo (pts[i][0], pts[i][1]);
        p.closeSubPath();
        return p;
    }();
    return path;
}

void draw (juce::Graphics& g, Id id, juce::Rectangle<float> area, juce::Colour c)
{
    // icons are designed on a square grid
    const auto side = juce::jmin (area.getWidth(), area.getHeight());
    const U u { area.withSizeKeepingCentre (side, side) };
    const float t = juce::jmax (1.2f, side * 0.09f); // stroke thickness
    g.setColour (c);

    switch (id)
    {
        case Id::none: break;

        case Id::arrowRight: tri (g, u, 0.3f, 0.2f, 0.3f, 0.8f, 0.75f, 0.5f); break;
        case Id::arrowDown:  tri (g, u, 0.2f, 0.32f, 0.8f, 0.32f, 0.5f, 0.75f); break;
        case Id::up:         tri (g, u, 0.2f, 0.7f, 0.8f, 0.7f, 0.5f, 0.25f); line (g, u, 0.5f, 0.5f, 0.5f, 0.9f, t); break;

        case Id::back:
        case Id::undo:
        {
            juce::Path p;
            p.addCentredArc (u.x (0.52f), u.y (0.56f), u.s (0.3f), u.s (0.28f), 0.0f,
                             -juce::MathConstants<float>::halfPi, juce::MathConstants<float>::pi * 0.95f, true);
            stroke (g, p, t);
            tri (g, u, 0.12f, 0.28f, 0.42f, 0.12f, 0.42f, 0.44f);
            break;
        }

        case Id::play:   tri (g, u, 0.28f, 0.18f, 0.28f, 0.82f, 0.82f, 0.5f); break;
        case Id::stop:   g.fillRect (u.rect (0.26f, 0.26f, 0.48f, 0.48f)); break;
        case Id::record: g.setColour (juce::Colour (0xffe5484d)); g.fillEllipse (u.rect (0.24f, 0.24f, 0.52f, 0.52f)); break;

        case Id::metronome:
        {
            juce::Path p;
            p.startNewSubPath (u.p (0.3f, 0.85f)); p.lineTo (u.p (0.42f, 0.2f)); p.lineTo (u.p (0.58f, 0.2f));
            p.lineTo (u.p (0.7f, 0.85f)); p.closeSubPath();
            stroke (g, p, t);
            line (g, u, 0.5f, 0.72f, 0.78f, 0.3f, t);
            g.fillEllipse (u.rect (0.68f, 0.33f, 0.14f, 0.14f));
            break;
        }

        case Id::waitInput:
            for (int i = 0; i < 3; ++i) g.fillRect (u.rect (0.1f + i * 0.14f, 0.3f, 0.09f, 0.42f));
            g.drawEllipse (u.rect (0.56f, 0.3f, 0.36f, 0.36f), t);
            line (g, u, 0.74f, 0.38f, 0.74f, 0.49f, t * 0.8f);
            line (g, u, 0.74f, 0.49f, 0.82f, 0.53f, t * 0.8f);
            break;

        case Id::countdown:
            g.setFont (juce::Font (juce::FontOptions (u.s (0.42f), juce::Font::italic)));
            g.drawText ("3.2.1", u.r, juce::Justification::centred);
            break;

        case Id::blend:
            for (int i = 0; i < 3; ++i) g.fillRect (u.rect (0.08f + i * 0.14f, 0.3f, 0.09f, 0.42f));
            line (g, u, 0.62f, 0.5f, 0.9f, 0.5f, t); line (g, u, 0.76f, 0.36f, 0.76f, 0.64f, t);
            break;

        case Id::loopRecord:
            for (int i = 0; i < 3; ++i) g.fillRect (u.rect (0.06f + i * 0.13f, 0.3f, 0.08f, 0.42f));
            {
                juce::Path p;
                p.addCentredArc (u.x (0.74f), u.y (0.52f), u.s (0.17f), u.s (0.17f), 0.0f, 0.3f, 5.6f, true);
                stroke (g, p, t);
                tri (g, u, 0.8f, 0.24f, 0.94f, 0.36f, 0.78f, 0.42f);
            }
            break;

        case Id::eye:
            g.drawEllipse (u.rect (0.12f, 0.32f, 0.76f, 0.36f), t);
            g.fillEllipse (u.rect (0.4f, 0.4f, 0.2f, 0.2f));
            break;

        case Id::reset:
        {
            juce::Path p;
            p.addCentredArc (u.x (0.5f), u.y (0.53f), u.s (0.3f), u.s (0.3f), 0.0f, 0.5f, 5.9f, true);
            stroke (g, p, t);
            tri (g, u, 0.5f, 0.1f, 0.72f, 0.24f, 0.5f, 0.36f);
            break;
        }

        case Id::mic:
            g.drawRoundedRectangle (u.rect (0.38f, 0.12f, 0.24f, 0.46f), u.s (0.12f), t);
            {
                juce::Path p;
                p.addCentredArc (u.x (0.5f), u.y (0.45f), u.s (0.22f), u.s (0.22f), 0.0f,
                                 juce::MathConstants<float>::halfPi, juce::MathConstants<float>::pi * 1.5f, true);
                stroke (g, p, t);
            }
            line (g, u, 0.5f, 0.67f, 0.5f, 0.86f, t);
            line (g, u, 0.36f, 0.86f, 0.64f, 0.86f, t);
            break;

        case Id::help:
            g.setFont (juce::Font (juce::FontOptions (u.s (0.82f), juce::Font::bold)));
            g.drawText ("?", u.r, juce::Justification::centred);
            break;

        case Id::save:
        case Id::saveNew:
        {
            auto body = u.rect (0.2f, 0.16f, 0.6f, 0.66f);
            g.drawRoundedRectangle (body, 1.5f, t);
            g.fillRect (u.rect (0.33f, 0.16f, 0.32f, 0.2f));
            g.fillRect (u.rect (0.3f, 0.52f, 0.4f, 0.24f));
            if (id == Id::saveNew)
                for (int i = 0; i < 4; ++i)
                    line (g, u, 0.38f + i * 0.08f, 0.88f - (i % 2) * 0.08f, 0.38f + i * 0.08f, 0.96f, t * 0.8f);
            break;
        }

        case Id::chat:
        {
            juce::Path p;
            p.addRoundedRectangle (u.rect (0.14f, 0.2f, 0.72f, 0.48f), u.s (0.2f));
            stroke (g, p, t);
            tri (g, u, 0.3f, 0.64f, 0.44f, 0.64f, 0.28f, 0.84f);
            for (int i = 0; i < 3; ++i) g.fillEllipse (u.rect (0.33f + i * 0.13f, 0.41f, 0.07f, 0.07f));
            break;
        }

        case Id::download:
            line (g, u, 0.5f, 0.14f, 0.5f, 0.56f, t * 1.3f);
            tri (g, u, 0.32f, 0.46f, 0.68f, 0.46f, 0.5f, 0.66f);
            {
                juce::Path p;
                p.startNewSubPath (u.p (0.16f, 0.6f)); p.lineTo (u.p (0.16f, 0.84f));
                p.lineTo (u.p (0.84f, 0.84f)); p.lineTo (u.p (0.84f, 0.6f));
                stroke (g, p, t);
            }
            break;

        case Id::minimise: line (g, u, 0.28f, 0.68f, 0.72f, 0.68f, t); break;
        case Id::maximise:
            g.drawRect (u.rect (0.24f, 0.34f, 0.42f, 0.4f), t);
            line (g, u, 0.36f, 0.24f, 0.78f, 0.24f, t * 0.8f);
            line (g, u, 0.78f, 0.24f, 0.78f, 0.62f, t * 0.8f);
            break;
        case Id::close:
            line (g, u, 0.28f, 0.28f, 0.72f, 0.72f, t * 1.2f);
            line (g, u, 0.72f, 0.28f, 0.28f, 0.72f, t * 1.2f);
            break;

        case Id::typingKeyboard:
            g.drawRect (u.rect (0.08f, 0.3f, 0.44f, 0.42f), t * 0.8f);
            for (int i = 0; i < 3; ++i) for (int j = 0; j < 2; ++j)
                g.fillRect (u.rect (0.13f + i * 0.12f, 0.37f + j * 0.15f, 0.07f, 0.08f));
            for (int i = 0; i < 3; ++i) g.fillRect (u.rect (0.58f + i * 0.12f, 0.3f, 0.08f, 0.42f));
            break;

        case Id::stepEdit:
            line (g, u, 0.18f, 0.5f, 0.66f, 0.5f, t * 1.6f);
            tri (g, u, 0.56f, 0.28f, 0.56f, 0.72f, 0.86f, 0.5f);
            break;

        case Id::slide:
        {
            juce::Path p;
            p.startNewSubPath (u.p (0.3f, 0.8f));
            p.cubicTo (u.p (0.5f, 0.78f), u.p (0.55f, 0.3f), u.p (0.75f, 0.2f));
            stroke (g, p, t * 1.3f);
            g.fillEllipse (u.rect (0.2f, 0.72f, 0.16f, 0.16f));
            break;
        }

        case Id::link:
            {
                juce::Path a, b;
                a.addRoundedRectangle (u.rect (0.12f, 0.4f, 0.44f, 0.2f), u.s (0.1f));
                b.addRoundedRectangle (u.rect (0.44f, 0.4f, 0.44f, 0.2f), u.s (0.1f));
                auto rot = juce::AffineTransform::rotation (-0.78f, u.x (0.5f), u.y (0.5f));
                a.applyTransform (rot); b.applyTransform (rot);
                stroke (g, a, t); stroke (g, b, t);
            }
            break;

        case Id::recFilter:
        {
            juce::Path p;
            p.startNewSubPath (u.p (0.25f, 0.82f)); p.lineTo (u.p (0.75f, 0.82f));
            p.lineTo (u.p (0.64f, 0.36f)); p.lineTo (u.p (0.36f, 0.36f)); p.closeSubPath();
            stroke (g, p, t);
            g.fillEllipse (u.rect (0.36f, 0.12f, 0.28f, 0.22f));
            break;
        }

        case Id::magnet:
        {
            juce::Path p;
            p.addCentredArc (u.x (0.5f), u.y (0.5f), u.s (0.24f), u.s (0.24f), 0.0f,
                             -juce::MathConstants<float>::halfPi, juce::MathConstants<float>::halfPi, true);
            p.applyTransform (juce::AffineTransform::rotation (juce::MathConstants<float>::pi, u.x (0.5f), u.y (0.5f)));
            stroke (g, p, t * 1.8f);
            line (g, u, 0.26f, 0.5f, 0.26f, 0.8f, t * 1.8f);
            line (g, u, 0.74f, 0.5f, 0.74f, 0.8f, t * 1.8f);
            break;
        }

        case Id::playlist:
            g.fillRect (u.rect (0.16f, 0.22f, 0.5f, 0.12f));
            g.fillRect (u.rect (0.32f, 0.44f, 0.52f, 0.12f));
            g.fillRect (u.rect (0.16f, 0.66f, 0.44f, 0.12f));
            break;

        case Id::pianoRoll:
            g.drawRect (u.rect (0.12f, 0.2f, 0.76f, 0.6f), t * 0.8f);
            for (int i = 1; i < 5; ++i) g.fillRect (u.rect (0.12f + i * 0.152f - 0.03f, 0.2f, 0.06f, 0.34f));
            break;

        case Id::channelRack:
            for (int row = 0; row < 3; ++row)
                for (int col = 0; col < 3; ++col)
                {
                    auto cell = u.rect (0.16f + col * 0.24f, 0.16f + row * 0.24f, 0.18f, 0.18f);
                    if ((row + col) % 2 == 0) g.fillRect (cell); else g.drawRect (cell, 1.0f);
                }
            break;

        case Id::mixer:
            for (int i = 0; i < 3; ++i)
            {
                const float x = 0.24f + i * 0.26f;
                line (g, u, x, 0.14f, x, 0.86f, t * 0.8f);
                g.fillRect (u.rect (x - 0.09f, 0.28f + (i == 1 ? 0.32f : i * 0.12f), 0.18f, 0.12f));
            }
            break;

        case Id::browser:
            g.drawRect (u.rect (0.12f, 0.14f, 0.3f, 0.2f), t * 0.8f);
            line (g, u, 0.24f, 0.34f, 0.24f, 0.78f, t * 0.8f);
            line (g, u, 0.24f, 0.52f, 0.5f, 0.52f, t * 0.8f);
            line (g, u, 0.24f, 0.78f, 0.5f, 0.78f, t * 0.8f);
            for (int i = 0; i < 2; ++i)
            {
                g.fillRect (u.rect (0.56f, 0.46f + i * 0.26f, 0.08f, 0.12f));
                g.fillRect (u.rect (0.7f, 0.46f + i * 0.26f, 0.16f, 0.12f));
            }
            break;

        case Id::project:
        case Id::file:
        {
            juce::Path p;
            p.startNewSubPath (u.p (0.24f, 0.12f)); p.lineTo (u.p (0.6f, 0.12f)); p.lineTo (u.p (0.78f, 0.3f));
            p.lineTo (u.p (0.78f, 0.88f)); p.lineTo (u.p (0.24f, 0.88f)); p.closeSubPath();
            if (id == Id::project) g.fillPath (p); else stroke (g, p, t * 0.8f);
            break;
        }

        case Id::plug:
            g.fillRoundedRectangle (u.rect (0.3f, 0.3f, 0.4f, 0.32f), u.s (0.06f));
            g.fillRect (u.rect (0.36f, 0.1f, 0.08f, 0.22f));
            g.fillRect (u.rect (0.56f, 0.1f, 0.08f, 0.22f));
            g.fillRect (u.rect (0.45f, 0.6f, 0.1f, 0.3f));
            break;

        case Id::catalog:
            g.drawRoundedRectangle (u.rect (0.14f, 0.34f, 0.72f, 0.5f), 2.0f, t);
            line (g, u, 0.5f, 0.1f, 0.5f, 0.58f, t * 1.2f);
            tri (g, u, 0.36f, 0.48f, 0.64f, 0.48f, 0.5f, 0.66f);
            break;

        case Id::globe:
            g.drawEllipse (u.rect (0.16f, 0.16f, 0.68f, 0.68f), t);
            g.drawEllipse (u.rect (0.36f, 0.16f, 0.28f, 0.68f), t * 0.8f);
            line (g, u, 0.16f, 0.5f, 0.84f, 0.5f, t * 0.8f);
            break;

        case Id::speaker:
        case Id::listen:
        case Id::mute:
        {
            juce::Path p;
            p.startNewSubPath (u.p (0.14f, 0.38f)); p.lineTo (u.p (0.3f, 0.38f)); p.lineTo (u.p (0.5f, 0.2f));
            p.lineTo (u.p (0.5f, 0.8f)); p.lineTo (u.p (0.3f, 0.62f)); p.lineTo (u.p (0.14f, 0.62f)); p.closeSubPath();
            g.fillPath (p);
            if (id == Id::mute)
            {
                line (g, u, 0.62f, 0.38f, 0.86f, 0.62f, t);
                line (g, u, 0.86f, 0.38f, 0.62f, 0.62f, t);
            }
            else if (id == Id::listen)
            {
                for (int i = 0; i < 2; ++i) g.fillRect (u.rect (0.62f + i * 0.14f, 0.3f, 0.07f, 0.4f));
            }
            else
            {
                juce::Path a;
                a.addCentredArc (u.x (0.48f), u.y (0.5f), u.s (0.26f), u.s (0.26f), 0.0f, 0.5f, 2.64f, true);
                stroke (g, a, t);
            }
            break;
        }

        case Id::graph:
            for (int i = 0; i < 5; ++i)
            {
                const float h = 0.2f + 0.12f * (float) ((i * 3) % 5);
                g.fillRect (u.rect (0.14f + i * 0.15f, 0.84f - h, 0.08f, h));
            }
            g.fillRect (u.rect (0.1f, 0.84f, 0.8f, 0.05f));
            break;

        case Id::steps:
            for (int i = 0; i < 3; ++i) g.fillRoundedRectangle (u.rect (0.1f + i * 0.28f, 0.28f, 0.22f, 0.44f), 1.5f);
            break;

        case Id::plus:
            line (g, u, 0.5f, 0.22f, 0.5f, 0.78f, t * 1.3f);
            line (g, u, 0.22f, 0.5f, 0.78f, 0.5f, t * 1.3f);
            break;

        case Id::star:
        {
            juce::Path p;
            p.addStar (u.p (0.5f, 0.53f), 5, u.s (0.16f), u.s (0.38f), 0.0f);
            g.fillPath (p);
            break;
        }

        case Id::folder:
        {
            juce::Path p;
            p.startNewSubPath (u.p (0.1f, 0.26f)); p.lineTo (u.p (0.38f, 0.26f)); p.lineTo (u.p (0.46f, 0.34f));
            p.lineTo (u.p (0.9f, 0.34f)); p.lineTo (u.p (0.9f, 0.8f)); p.lineTo (u.p (0.1f, 0.8f)); p.closeSubPath();
            stroke (g, p, t * 0.8f);
            break;
        }

        case Id::wave:
        {
            const float hs[] = { 0.1f, 0.25f, 0.4f, 0.22f, 0.34f, 0.14f, 0.28f, 0.08f };
            for (int i = 0; i < 8; ++i)
                line (g, u, 0.12f + i * 0.108f, 0.5f - hs[i], 0.12f + i * 0.108f, 0.5f + hs[i], t);
            break;
        }

        case Id::automation:
            line (g, u, 0.2f, 0.76f, 0.8f, 0.24f, t);
            g.drawEllipse (u.rect (0.1f, 0.66f, 0.2f, 0.2f), t);
            g.drawEllipse (u.rect (0.7f, 0.14f, 0.2f, 0.2f), t);
            break;

        case Id::keys:
            g.fillRect (u.rect (0.1f, 0.24f, 0.8f, 0.56f));
            g.setColour (juce::Colour (0xff1a1f22));
            for (int i = 1; i < 4; ++i) g.fillRect (u.rect (0.1f + i * 0.2f - 0.015f, 0.24f, 0.03f, 0.56f));
            for (int i : { 0, 1, 3 }) g.fillRect (u.rect (0.24f + i * 0.2f, 0.24f, 0.1f, 0.32f));
            break;

        case Id::pencil:
        {
            juce::Path p;
            p.startNewSubPath (u.p (0.2f, 0.8f)); p.lineTo (u.p (0.26f, 0.6f)); p.lineTo (u.p (0.66f, 0.2f));
            p.lineTo (u.p (0.8f, 0.34f)); p.lineTo (u.p (0.4f, 0.74f)); p.closeSubPath();
            stroke (g, p, t);
            break;
        }

        case Id::brush:
            line (g, u, 0.76f, 0.16f, 0.44f, 0.52f, t * 1.6f);
            g.fillEllipse (u.rect (0.18f, 0.5f, 0.32f, 0.32f));
            break;

        case Id::erase:
            g.drawEllipse (u.rect (0.2f, 0.2f, 0.6f, 0.6f), t * 1.2f);
            line (g, u, 0.3f, 0.7f, 0.7f, 0.3f, t * 1.2f);
            break;

        case Id::slip:
            line (g, u, 0.16f, 0.5f, 0.84f, 0.5f, t * 1.2f);
            tri (g, u, 0.1f, 0.5f, 0.3f, 0.32f, 0.3f, 0.68f);
            tri (g, u, 0.9f, 0.5f, 0.7f, 0.32f, 0.7f, 0.68f);
            break;

        case Id::slice:
        {
            juce::Path p;
            p.startNewSubPath (u.p (0.3f, 0.16f)); p.lineTo (u.p (0.62f, 0.16f)); p.lineTo (u.p (0.62f, 0.86f));
            p.lineTo (u.p (0.3f, 0.5f)); p.closeSubPath();
            g.fillPath (p);
            line (g, u, 0.72f, 0.1f, 0.72f, 0.9f, t * 0.8f);
            break;
        }

        case Id::select:
        {
            const float d[] = { 0.12f, 0.88f };
            for (float a : d) for (float b : d)
            {
                line (g, u, a, b, a + (a < 0.5f ? 0.22f : -0.22f), b, t);
                line (g, u, a, b, a, b + (b < 0.5f ? 0.22f : -0.22f), t);
            }
            break;
        }

        case Id::zoom:
        case Id::search:
            g.drawEllipse (u.rect (0.16f, 0.14f, 0.5f, 0.5f), t * 1.2f);
            line (g, u, 0.58f, 0.58f, 0.84f, 0.84f, t * 1.8f);
            break;

        case Id::tags:
        {
            juce::Path p;
            p.startNewSubPath (u.p (0.14f, 0.2f)); p.lineTo (u.p (0.5f, 0.2f)); p.lineTo (u.p (0.86f, 0.56f));
            p.lineTo (u.p (0.56f, 0.86f)); p.lineTo (u.p (0.14f, 0.44f)); p.closeSubPath();
            stroke (g, p, t);
            break;
        }

        case Id::cloud:
            g.fillEllipse (u.rect (0.12f, 0.42f, 0.36f, 0.32f));
            g.fillEllipse (u.rect (0.3f, 0.26f, 0.42f, 0.44f));
            g.fillEllipse (u.rect (0.54f, 0.42f, 0.34f, 0.32f));
            g.fillRect (u.rect (0.28f, 0.56f, 0.44f, 0.18f));
            break;

        case Id::preset:
            g.drawRect (u.rect (0.14f, 0.22f, 0.5f, 0.5f), t);
            tri (g, u, 0.6f, 0.4f, 0.6f, 0.86f, 0.9f, 0.63f);
            break;

        case Id::sampler:
            g.drawRoundedRectangle (u.rect (0.12f, 0.22f, 0.76f, 0.56f), 2.0f, t);
            for (int i = 0; i < 5; ++i)
            {
                const float h = 0.06f + 0.05f * (float) ((i * 2) % 5);
                line (g, u, 0.26f + i * 0.12f, 0.5f - h, 0.26f + i * 0.12f, 0.5f + h, t);
            }
            break;
    }
}
}
