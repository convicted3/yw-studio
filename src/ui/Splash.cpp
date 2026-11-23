#include "ui/Splash.h"

#include <atomic>
#include <mutex>
#include <optional>
#include <thread>

#include "core/StartupSound.h"
#include "engine/PluginHost.h"
#include "ui/I18n.h"
#include "ui/Theme.h"
#include "ui/WindowOwner.h"

#include <cstring>

#if JUCE_WINDOWS
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
 #endif
 #include <windows.h>
 #undef near // (windows.h; the drawing below has a variable of that name)
 #undef far
#endif

namespace yw
{
namespace
{
double clamp01 (double x) { return juce::jlimit (0.0, 1.0, x); }
double easeOutCubic (double u) { return 1.0 - std::pow (1.0 - clamp01 (u), 3.0); }

double easeOutBack (double u)
{
    u = clamp01 (u);
    constexpr double c1 = 1.5, c3 = c1 + 1.0;
    return 1.0 + c3 * std::pow (u - 1.0, 3.0) + c1 * std::pow (u - 1.0, 2.0);
}

// a ball dropped on the floor: 0 = up, 1 = resting; it touches the floor at u = 1/2.75, 2/2.75, 2.5/2.75, 1
double easeOutBounce (double u)
{
    u = clamp01 (u);
    constexpr double n1 = 7.5625, d1 = 2.75;

    if (u < 1.0 / d1)
        return n1 * u * u;
    auto arc = [] (double v, double centre, double floor) { return n1 * (v - centre) * (v - centre) + floor; };
    if (u < 2.0 / d1)
        return arc (u, 1.5 / d1, 0.75);
    if (u < 2.5 / d1)
        return arc (u, 2.25 / d1, 0.9375);
    return arc (u, 2.625 / d1, 0.984375);
}

// the drop lasts so that its first touch is the sound's landing "pop"
constexpr double dropTime = startup::impactTime * 2.75;
constexpr double touches[] = { dropTime / 2.75, dropTime * 2.0 / 2.75, dropTime * 2.5 / 2.75 };
constexpr double touchStrength[] = { 1.0, 0.35, 0.12 };

double impactAt (double t)
{
    double k = 0.0;
    for (int i = 0; i < 3; ++i)
        if (t >= touches[i])
            k += touchStrength[i] * std::exp (-(t - touches[i]) / 0.07);
    return k;
}
} // namespace

// --- options --------------------------------------------------------------------------------------

StartupOptions StartupOptions::load()
{
    StartupOptions options;
    juce::StringArray lines;
    lines.addLines (PluginHost::settingsFolder().getChildFile ("ui-options.txt").loadFileAsString());

    for (const auto& line : lines)
    {
        const auto key = line.upToFirstOccurrenceOf ("=", false, false).trim();
        const auto on = line.fromFirstOccurrenceOf ("=", false, false).trim() != "0";

        if (key == "splash")
            options.splash = on;
        else if (key == "startupSound")
            options.sound = on;
    }

    return options;
}

std::shared_ptr<const SampleData> makeStartupSample()
{
    auto sound = startup::makeSound (48000.0);
    auto sample = std::make_shared<SampleData>();
    sample->left = std::move (sound.left);
    sample->right = std::move (sound.right);
    sample->sampleRate = sound.sampleRate;
    return sample;
}

// --- the splash -----------------------------------------------------------------------------------

SplashScreen::SplashScreen()
    : logoMark (logoImage())   // the app icon by w3teal (512 px, src/ui/LogoData.h)
{
    setOpaque (false);
    setSize (width, height);
    status = tr ("Starting the audio engine...");
}

SplashScreen::~SplashScreen()
{
    stopTimer();
}

void SplashScreen::showOnDesktop()
{
    if (const auto* display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
        setCentrePosition (display->userArea.getCentre());

    addToDesktop (0); // no title bar, no taskbar entry
    setAlwaysOnTop (true);
    setVisible (true);
    toFront (false);
    startTimerHz (60);
}

void SplashScreen::startClock()
{
    clockStartMs = juce::Time::getMillisecondCounterHiRes();
    time = 0.0;
}

double SplashScreen::remainingDuration (double sinceProcessStart, double landing)
{
    return juce::jmax (landing, minimumDuration - juce::jmax (0.0, sinceProcessStart));
}

double SplashScreen::getElapsed() const
{
    return clockStartMs < 0.0 ? 0.0 : (juce::Time::getMillisecondCounterHiRes() - clockStartMs) / 1000.0;
}

void SplashScreen::setStatus (const juce::String& text)
{
    status = text;
    repaint();
}

void SplashScreen::finish (std::function<void()> whenGone)
{
    onGone = std::move (whenGone);
    fadeStartMs = juce::Time::getMillisecondCounterHiRes();

    if (! isTimerRunning())
        startTimerHz (60);
}

void SplashScreen::setTimeForSnapshot (double seconds)
{
    time = seconds;
    repaint();
}

void SplashScreen::timerCallback()
{
    time = getElapsed();

    if (fadeStartMs >= 0.0)
    {
        const auto f = (juce::Time::getMillisecondCounterHiRes() - fadeStartMs) / 1000.0 / fadeOutTime;
        setAlpha ((float) (1.0 - clamp01 (f)));

        if (f >= 1.0)
        {
            stopTimer();
            setVisible (false);
            removeFromDesktop();

            if (auto gone = std::move (onGone))
                juce::MessageManager::callAsync (std::move (gone)); // may delete this splash
            return;
        }
    }

    repaint();
}

void SplashScreen::paint (juce::Graphics& g)
{
    if (clockStartMs >= 0.0 && fadeStartMs < 0.0) // (the drop's frames: see getDropFrames)
    {
        const auto now = juce::Time::getMillisecondCounterHiRes();
        if (getElapsed() <= 1.05)
        {
            if (lastDropPaintMs >= 0.0)
                longestDropGapMs = juce::jmax (longestDropGapMs, now - lastDropPaintMs);
            ++dropFrames;
        }
        lastDropPaintMs = now;
    }

    SplashFrame frame;
    frame.t = time;
    frame.barDuration = barDuration;
    frame.status = status;
    frame.subtitle = tr ("Digital audio workstation");
    frame.version = tr ("Version {0}", { JUCE_APPLICATION_VERSION_STRING });
    frame.logo = &logoMark;
    drawSplashFrame (g, frame);

    if (auto first = std::exchange (onFirstFrame, nullptr))
    {
        firstFrameTime = std::chrono::steady_clock::now();
        juce::MessageManager::callAsync (std::move (first)); // (after this frame is on the screen)
    }
}

std::array<juce::Font, 4> makeSplashFonts()
{
    std::array<juce::Font, 4> fonts { juce::Font (juce::FontOptions (78.0f, juce::Font::bold)), juce::Font (juce::FontOptions (34.0f, juce::Font::plain)),
                                      juce::Font (theme::font (theme::type::title)), juce::Font (theme::font (theme::type::small)) };
    for (auto& f : fonts)
        f.getTypefacePtr(); // (looked up here, once)
    return fonts;
}

namespace
{
// The YW mark (a 304 x 192 box), split into its Y and W halves at x = 111 for the drop.
juce::Path ywLogoPath()
{
    static const float pts[][2] = { { 261.0f, 183.4f }, { 220.5f, 165.0f }, { 198.0f, 75.9f }, { 174.0f, 154.9f }, { 135.4f, 154.0f },
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
}

constexpr float logoW = 304.0f, logoH = 192.0f, splitX = 111.0f, travel = 300.0f;
constexpr double fallTime = 0.165;
constexpr double wDelay = startup::impactTime - fallTime; // the W lands with the start-up sound's hit
constexpr double yDelay = wDelay - 0.055;

// where a piece is (0 = resting) and how it is squashed, t seconds into the splash
struct Piece { float offset = travel; float squash = 1.0f; float speed = 0.0f; bool landed = false; double since = 0.0; };

Piece pieceAt (double t, double delay)
{
    Piece p;
    const auto u = (t - delay) / fallTime;
    if (u < 0.0)
        return p;
    if (u < 1.0)
    {
        p.offset = travel * (float) (1.0 - std::pow (u, 2.6));
        p.speed = travel * (float) (2.6 * std::pow (u, 1.6) / fallTime);
        return p;
    }
    const auto tau = t - delay - fallTime;
    const auto wave = 2.0 * juce::MathConstants<double>::pi * 7.5 * tau;
    p.offset = -travel * 0.075f * (float) (std::exp (-15.0 * tau) * std::sin (wave));
    p.squash = 1.0f - 0.16f * (float) (std::exp (-22.0 * tau) * std::cos (wave));
    p.landed = true;
    p.since = tau;
    return p;
}
} // namespace

void drawSplashFrame (juce::Graphics& g, const SplashFrame& frame)
{
    // YW: no card, no words, nothing but the mark on a transparent window. The Y drops in from the top and the W
    // from the bottom, they hit, bounce and lock together, then "yolo.su" fades in under them.
    const auto t = frame.t;
    std::optional<std::array<juce::Font, 4>> made;
    if (frame.fonts == nullptr)
        made = makeSplashFonts();
    const auto* fonts = frame.fonts != nullptr ? frame.fonts : made->data();

    static const juce::Path logo = ywLogoPath();
    const auto W = (float) SplashScreen::width, H = (float) SplashScreen::height;
    const auto ox = (W - logoW) / 2.0f, oy = 34.0f;

    const auto y = pieceAt (t, yDelay), w = pieceAt (t, wDelay);

    // a short camera shake from the hits
    auto shake = 0.0f;
    for (const auto* p : { &y, &w })
        if (p->landed)
            shake += 4.0f * (float) (std::exp (-18.0 * p->since) * std::sin (p->since * 2.0 * juce::MathConstants<double>::pi * 23.0));

    juce::Graphics::ScopedSaveState whole (g);
    g.addTransform (juce::AffineTransform::translation (ox + shake * 0.4f, oy + shake));

    // impact glow behind the mark
    {
        auto glow = 0.0f;
        for (const auto* p : { &y, &w })
            if (p->landed)
                glow += 0.45f * (float) std::exp (-6.0 * p->since);
        const auto settle = (float) clamp01 ((t - wDelay - fallTime) / 0.6);
        glow += 0.10f * settle;
        if (glow > 0.003f)
        {
            juce::ColourGradient grad (juce::Colours::white.withAlpha (juce::jmin (0.6f, glow)), logoW / 2.0f, logoH / 2.0f,
                                       juce::Colours::white.withAlpha (0.0f), logoW / 2.0f + 190.0f, logoH / 2.0f, true);
            g.setGradientFill (grad);
            g.fillEllipse (logoW / 2.0f - 190.0f, logoH / 2.0f - 150.0f, 380.0f, 300.0f);
        }
    }

    // once both halves have stopped, the mark is drawn whole (no seam)
    const auto merged = y.landed && w.landed && w.since > 0.35;

    const auto drawPiece = [&] (const Piece& p, bool isY, float alpha)
    {
        const auto dir = isY ? -1.0f : 1.0f; // Y comes from above, W from below
        const auto clipRect = isY ? juce::Rectangle<float> (-20.0f, -20.0f, splitX + 0.8f + 20.0f, logoH + 40.0f)
                                  : juce::Rectangle<float> (splitX, -20.0f, logoW - splitX + 20.0f, logoH + 40.0f);
        const auto pivotY = isY ? logoH : 0.0f; // squash against the side it lands on

        const auto drawAt = [&] (float offset, float a)
        {
            juce::Graphics::ScopedSaveState s (g);
            g.addTransform (juce::AffineTransform::scale (1.0f + (1.0f - p.squash) * 0.5f, p.squash, logoW / 2.0f, pivotY)
                                .translated (0.0f, dir * offset));
            g.reduceClipRegion (clipRect.toNearestInt());
            g.setColour (juce::Colours::white.withAlpha (a));
            g.fillPath (logo);
        };

        // motion trail while falling
        if (! p.landed && p.offset < travel)
            for (int k = 3; k >= 1; --k)
                drawAt (p.offset + (float) k * 0.012f * p.speed, alpha * 0.12f * (float) (4 - k) / 3.0f);

        if (p.offset < travel)
            drawAt (p.offset, alpha);
    };

    if (merged)
    {
        g.setColour (juce::Colours::white);
        g.fillPath (logo);
    }
    else
    {
        drawPiece (y, true, 1.0f);
        drawPiece (w, false, 1.0f);
    }

    // "yolo.su": fades in under the mark while its letters close up
    {
        const auto p = (float) easeOutCubic ((t - 0.50) / 0.46);
        if (p > 0.0f)
        {
            const juce::String text ("yolo.su");
            const auto& f = fonts[1];
            const auto tracking = 26.0f + (5.0f - 26.0f) * p;
            auto total = 0.0f;
            for (int i = 0; i < text.length(); ++i)
                total += juce::GlyphArrangement::getStringWidth (f, text.substring (i, i + 1));
            total += tracking * (float) (text.length() - 1);
            auto x = (logoW - total) / 2.0f;
            const auto baseline = logoH + 64.0f;
            g.setFont (f);
            g.setColour (juce::Colours::white.withAlpha (0.92f * p));
            for (int i = 0; i < text.length(); ++i)
            {
                const auto letter = text.substring (i, i + 1);
                g.drawSingleLineText (letter, juce::roundToInt (x), juce::roundToInt (baseline));
                x += juce::GlyphArrangement::getStringWidth (f, letter) + tracking;
            }
        }
    }
    juce::ignoreUnused (H);
}
// --- the card on a thread of its own (Windows, round 190hd) --------------------------------------
//
// The card was a JUCE window on the message thread: the start-up could not build the main window or open the audio
// device while the logo dropped (each would stop the drop), and its logo was the process's first Direct2D image, so
// the Direct3D 11 + Direct2D device (~0.35-0.4 s) was made before the card could show. Here the same drawing
// (drawSplashFrame) runs on a thread of its own with the CPU (JUCE's software renderer, JUCE patch 30: no Direct2D
// image on this thread) into a layered window of its own (UpdateLayeredWindow, as JUCE's own transparent windows do
// without Direct2D): the card shows within a few tens of milliseconds and its 60 frames a second no longer depend on
// what the message thread does. Same size, place, drop, timing and fade as the JUCE card. YW_SPLASH_THREAD=0: the
// JUCE card.
#if JUCE_WINDOWS
} // namespace yw

namespace juce
{
void ywSoftwareImagesOnThisThread (bool on); // cmake/YwPatchJuce.cmake fix 30
}

namespace yw
{
namespace
{
double steadySeconds()
{
    return std::chrono::duration<double> (std::chrono::steady_clock::now().time_since_epoch()).count();
}

class ThreadedSplash final : public StartupSplash
{
public:
    ThreadedSplash()
        : fonts (makeSplashFonts()),
          subtitle (tr ("Digital audio workstation")),
          version (tr ("Version {0}", { JUCE_APPLICATION_VERSION_STRING })),
          dropStatus (tr ("Loading the workspace...")),
          status (tr ("Starting the audio engine..."))
    {
        wake = CreateEventW (nullptr, FALSE, FALSE, nullptr);
    }

    ~ThreadedSplash() override
    {
        quit = true;
        if (wake != nullptr)
            SetEvent (wake);
        if (thread.joinable())
            thread.join();
        if (wake != nullptr)
            CloseHandle (wake);
    }

    void showOnDesktop() override
    {
        if (! thread.joinable())
            thread = std::thread ([this] { run(); });
    }

    void startClock() override { clockStart.store (steadySeconds()); }

    double getElapsed() const override
    {
        const auto start = clockStart.load();
        return start < 0.0 ? 0.0 : steadySeconds() - start;
    }

    void setStatus (const juce::String& text) override
    {
        const std::scoped_lock hold (lock);
        status = text;
    }

    void setBarDuration (double seconds) override { barDuration.store (seconds); }

    void raise() override
    {
        raiseAsked = true;
        SetEvent (wake);
    }

    void finish (std::function<void()> whenGone) override
    {
        {
            const std::scoped_lock hold (lock);
            onGone = std::move (whenGone);
        }
        fadeStart.store (steadySeconds());
        SetEvent (wake);
    }

    int getDropFrames() const override { return dropFrames.load(); }
    double getLongestDropGapMs() const override { return longestDropGapMs.load(); }

    std::chrono::steady_clock::time_point getFirstFrameTime() const override
    {
        return std::chrono::steady_clock::time_point (std::chrono::steady_clock::duration (firstFrameTicks.load()));
    }

    bool drawsOnItsOwnThread() const override { return true; }

    void whenLanded (double seconds, std::function<void()> then) override
    {
        {
            const std::scoped_lock hold (lock);
            onLanded = std::move (then);
        }
        landingAt.store (seconds);
    }

    double getLongestDropGapAt() const override { return longestDropGapAt.load(); }

    juce::String getLongestDropGapDetail() const override
    {
        const std::scoped_lock hold (lock);
        return longestGapDetail;
    }

private:
    // runs `f` on the message thread unless this card is gone by then (it is deleted there)
    void post (std::function<void (ThreadedSplash&)> f)
    {
        juce::MessageManager::callAsync ([weak = std::weak_ptr<int> (alive), self = this, f = std::move (f)] {
            if (! weak.expired())
                f (*self);
        });
    }

    void run()
    {
        juce::ywSoftwareImagesOnThisThread (true);
        SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
        // its window in physical pixels, like JUCE's (per-monitor DPI awareness)
        using SetContext = HANDLE (WINAPI*) (HANDLE);
        if (auto* user32 = GetModuleHandleW (L"user32.dll"))
            if (auto set = reinterpret_cast<SetContext> (reinterpret_cast<void*> (GetProcAddress (user32, "SetThreadDpiAwarenessContext"))))
                set (reinterpret_cast<HANDLE> (static_cast<std::intptr_t> (-4))); // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2

        const auto logo = decodeLogoImage();

        // centred in the main display's work area (JUCE's card: the primary display's user area)
        const auto monitor = MonitorFromPoint (POINT { 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
        MONITORINFO info {};
        info.cbSize = sizeof (info);
        GetMonitorInfoW (monitor, &info);
        UINT dpiX = 96, dpiY = 96;
        using GetDpi = HRESULT (WINAPI*) (HMONITOR, int, UINT*, UINT*);
        if (auto* shcore = LoadLibraryW (L"shcore.dll"))
            if (auto get = reinterpret_cast<GetDpi> (reinterpret_cast<void*> (GetProcAddress (shcore, "GetDpiForMonitor"))))
                if (FAILED (get (monitor, 0 /* MDT_EFFECTIVE_DPI */, &dpiX, &dpiY)))
                    dpiX = 96;
        const auto scale = juce::jmax (1.0, (double) dpiX / 96.0);
        const auto w = juce::roundToInt (SplashScreen::width * scale), h = juce::roundToInt (SplashScreen::height * scale);
        const auto& work = info.rcWork;
        const auto x = (work.left + work.right) / 2 - w / 2, y = (work.top + work.bottom) / 2 - h / 2;

        const auto instance = GetModuleHandleW (nullptr);
        WNDCLASSEXW windowClass {};
        windowClass.cbSize = sizeof (windowClass);
        windowClass.lpfnWndProc = DefWindowProcW;
        windowClass.hInstance = instance;
        windowClass.hCursor = LoadCursorW (nullptr, MAKEINTRESOURCEW (32512)); // (IDC_ARROW)
        windowClass.lpszClassName = L"YWSplashCard";
        RegisterClassExW (&windowClass);
        // no title (nothing looks for it), no taskbar entry, never activated
        const auto window = CreateWindowExW (WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, windowClass.lpszClassName, L"",
                                             WS_POPUP, x, y, w, h, nullptr, nullptr, instance, nullptr);

        BITMAPINFO bitmapInfo {};
        bitmapInfo.bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
        bitmapInfo.bmiHeader.biWidth = w;
        bitmapInfo.bmiHeader.biHeight = -h; // (top-down)
        bitmapInfo.bmiHeader.biPlanes = 1;
        bitmapInfo.bmiHeader.biBitCount = 32;
        bitmapInfo.bmiHeader.biCompression = BI_RGB;
        void* bits = nullptr;
        const auto dib = window != nullptr ? CreateDIBSection (nullptr, &bitmapInfo, DIB_RGB_COLORS, &bits, nullptr, 0) : nullptr;
        const auto memoryDc = CreateCompatibleDC (nullptr);
        const auto oldBitmap = dib != nullptr ? SelectObject (memoryDc, dib) : nullptr;
        // (high resolution where Windows has it: 60 frames a second on the dot)
        auto timer = CreateWaitableTimerExW (nullptr, nullptr, 0x00000002 /* CREATE_WAITABLE_TIMER_HIGH_RESOLUTION */, TIMER_ALL_ACCESS);
        if (timer == nullptr)
            timer = CreateWaitableTimerW (nullptr, FALSE, nullptr);

        if (window == nullptr || dib == nullptr || bits == nullptr)
        {
            // (no card: the start-up goes on all the same)
            firstFrameTicks.store (std::chrono::steady_clock::now().time_since_epoch().count());
            post ([] (ThreadedSplash& s) { if (auto first = std::exchange (s.onFirstFrame, nullptr)) first(); });
            while (! quit && fadeStart.load() < 0.0)
                WaitForSingleObject (wake, 50);
            if (! quit)
                post ([] (ThreadedSplash& s) { s.gone(); });
            cleanUp (window, memoryDc, oldBitmap, dib, timer);
            return;
        }

        juce::Image frame (juce::Image::ARGB, w, h, true, juce::SoftwareImageType());
        // every glyph the card will draw, once, before its first frame (an outline looked up mid-drop waited for the
        // font system while the message thread loaded the window's fonts: a frame of ~0.1 s)
        {
            SplashFrame warm;
            warm.t = 1.6;
            warm.status = dropStatus + " " + status;
            warm.subtitle = subtitle;
            warm.version = version;
            warm.logo = &logo;
            warm.fonts = fonts.data();
            juce::Graphics g (frame);
            g.addTransform (juce::AffineTransform::scale ((float) scale));
            drawSplashFrame (g, warm);
        }
        constexpr double period = 1.0 / 60.0;
        auto next = steadySeconds();
        auto shown = false, faded = false;
        auto lastDropPaint = -1.0;

        while (! quit)
        {
            const auto now = steadySeconds();
            const auto start = clockStart.load(), fade = fadeStart.load();
            const auto t = start < 0.0 ? 0.0 : now - start;
            auto alpha = 1.0;
            if (fade >= 0.0)
            {
                const auto f = (now - fade) / SplashScreen::fadeOutTime;
                alpha = 1.0 - clamp01 (f);
                if (f >= 1.0)
                {
                    faded = true;
                    break;
                }
            }
            if (start >= 0.0 && fade < 0.0 && t <= 1.05) // (the drop's frames, as the JUCE card counts them)
            {
                if (lastDropPaint >= 0.0 && (now - lastDropPaint) * 1000.0 > longestDropGapMs.load())
                {
                    longestDropGapMs.store ((now - lastDropPaint) * 1000.0);
                    longestDropGapAt.store (t);
                    const std::scoped_lock hold (lock); // (what the frame before it spent its time on)
                    longestGapDetail = "draw " + juce::String (spent[0], 1) + " ms, show " + juce::String (spent[1], 1) + " ms, messages "
                                       + juce::String (spent[2], 1) + " ms, wait " + juce::String (spent[3], 1) + " ms";
                }
                ++dropFrames;
                lastDropPaint = now;
            }
            if (const auto landing = landingAt.load(); start >= 0.0 && landing >= 0.0 && t >= landing)
            {
                landingAt.store (-1.0);
                post ([] (ThreadedSplash& s) {
                    std::function<void()> then;
                    {
                        const std::scoped_lock hold (s.lock);
                        then = std::move (s.onLanded);
                    }
                    if (then)
                        then(); // (may delete this card)
                });
            }

            auto mark = steadySeconds();
            const auto lap = [&mark] (double& into) { const auto n = steadySeconds(); into = (n - mark) * 1000.0; mark = n; };
            SplashFrame look;
            look.t = t;
            look.barDuration = barDuration.load();
            {
                const std::scoped_lock hold (lock);
                look.status = status;
            }
            look.subtitle = subtitle;
            look.version = version;
            look.logo = &logo;
            look.fonts = fonts.data();
            frame.clear (frame.getBounds());
            {
                juce::Graphics g (frame);
                g.addTransform (juce::AffineTransform::scale ((float) scale));
                drawSplashFrame (g, look);
            }
            {
                const juce::Image::BitmapData pixels (frame, juce::Image::BitmapData::readOnly);
                for (int row = 0; row < h; ++row) // (premultiplied BGRA, as UpdateLayeredWindow wants)
                    std::memcpy (static_cast<std::uint8_t*> (bits) + (std::size_t) row * (std::size_t) w * 4, pixels.getLinePointer (row), (std::size_t) w * 4);
            }
            lap (spent[0]);
            POINT source { 0, 0 }, place { x, y };
            SIZE size { w, h };
            BLENDFUNCTION blend { AC_SRC_OVER, 0, (BYTE) juce::roundToInt (alpha * 255.0), AC_SRC_ALPHA };
            UpdateLayeredWindow (window, nullptr, &place, &size, memoryDc, &source, 0, &blend, ULW_ALPHA);
            if (raiseAsked.exchange (false))
                SetWindowPos (window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            if (! std::exchange (shown, true))
            {
                ShowWindow (window, SW_SHOWNOACTIVATE);
                firstFrameTicks.store (std::chrono::steady_clock::now().time_since_epoch().count());
                post ([] (ThreadedSplash& s) { if (auto first = std::exchange (s.onFirstFrame, nullptr)) first(); });
            }
            lap (spent[1]);

            MSG message;
            while (PeekMessageW (&message, nullptr, 0, 0, PM_REMOVE))
            {
                TranslateMessage (&message);
                DispatchMessageW (&message);
            }
            lap (spent[2]);

            // the next tick of a 60 Hz clock (a tick missed is skipped, as JUCE's timer does)
            next += period;
            const auto later = steadySeconds();
            while (next <= later)
                next += period;
            LARGE_INTEGER due;
            due.QuadPart = -juce::jmax ((LONGLONG) 1, (LONGLONG) ((next - later) * 1.0e7)); // (relative, 100 ns units)
            // (the timeout: the tick plus a little, in case the timer never signals; never a long wait)
            const auto timeoutMs = (DWORD) juce::jmax (1, (int) std::ceil ((next - later) * 1000.0) + 2);
            if (timer != nullptr && SetWaitableTimer (timer, &due, 0, nullptr, nullptr, FALSE))
            {
                const HANDLE waitFor[] = { timer, wake };
                MsgWaitForMultipleObjects (2, waitFor, FALSE, timeoutMs, QS_ALLINPUT);
            }
            else
            {
                MsgWaitForMultipleObjects (1, &wake, FALSE, timeoutMs, QS_ALLINPUT);
            }
            lap (spent[3]);
        }

        ShowWindow (window, SW_HIDE);
        cleanUp (window, memoryDc, oldBitmap, dib, timer);
        if (faded && ! quit)
            post ([] (ThreadedSplash& s) { s.gone(); });
    }

    static void cleanUp (HWND window, HDC memoryDc, HGDIOBJ oldBitmap, HBITMAP dib, HANDLE timer)
    {
        if (window != nullptr)
            DestroyWindow (window);
        if (oldBitmap != nullptr)
            SelectObject (memoryDc, oldBitmap);
        if (dib != nullptr)
            DeleteObject (dib);
        if (memoryDc != nullptr)
            DeleteDC (memoryDc);
        if (timer != nullptr)
            CloseHandle (timer);
    }

    // (message thread) the fade has ended
    void gone()
    {
        std::function<void()> then;
        {
            const std::scoped_lock hold (lock);
            then = std::move (onGone);
        }
        if (then)
            then(); // (may delete this card)
    }

    const std::array<juce::Font, 4> fonts;
    const juce::String subtitle, version, dropStatus;
    mutable std::mutex lock; // guards status, onGone, onLanded and longestGapDetail
    juce::String status;
    std::function<void()> onGone, onLanded;
    std::atomic<double> landingAt { -1.0 }, longestDropGapAt { -1.0 };
    double spent[4] {}; // (the splash thread) the last frame's draw, show, messages, wait, ms
    juce::String longestGapDetail; // (guarded by lock)
    std::atomic<double> clockStart { -1.0 }, fadeStart { -1.0 }, barDuration { SplashScreen::minimumDuration };
    std::atomic<bool> quit { false }, raiseAsked { false };
    std::atomic<int> dropFrames { 0 };
    std::atomic<double> longestDropGapMs { 0.0 };
    std::atomic<std::chrono::steady_clock::rep> firstFrameTicks { 0 };
    HANDLE wake = nullptr;
    std::shared_ptr<int> alive = std::make_shared<int> (0);
    std::thread thread; // (last)
};
} // namespace
#endif

juce::Array<juce::File> writeSplashShots (const juce::File& folder)
{
    juce::Array<juce::File> written;
    folder.createDirectory();
    const auto fonts = makeSplashFonts();
    const auto gpuLogo = decodeLogoImage();
#if JUCE_WINDOWS
    juce::ywSoftwareImagesOnThisThread (true);
#endif
    const auto cpuLogo = decodeLogoImage();
#if JUCE_WINDOWS
    juce::ywSoftwareImagesOnThisThread (false);
#endif
    for (const auto scale : { 1.0f, 1.5f })
        for (const auto t : { 0.0, 0.29, 0.58, 1.0, 1.6 })
            for (const auto cpu : { false, true })
            {
                const juce::SoftwareImageType software;
                const juce::NativeImageType native;
                const juce::ImageType& type = cpu ? static_cast<const juce::ImageType&> (software) : static_cast<const juce::ImageType&> (native);
                juce::Image image (juce::Image::ARGB, juce::roundToInt (SplashScreen::width * scale), juce::roundToInt (SplashScreen::height * scale), true, type);
                {
                    juce::Graphics g (image);
                    g.addTransform (juce::AffineTransform::scale (scale));
                    SplashFrame frame;
                    frame.t = t;
                    frame.barDuration = 1.05;
                    frame.status = tr ("Loading the workspace...");
                    frame.subtitle = tr ("Digital audio workstation");
                    frame.version = tr ("Version {0}", { JUCE_APPLICATION_VERSION_STRING });
                    frame.logo = cpu ? &cpuLogo : &gpuLogo;
                    frame.fonts = fonts.data();
                    drawSplashFrame (g, frame);
                }
                const auto file = folder.getChildFile ("splash-" + juce::String (juce::roundToInt (scale * 100)) + "-" + juce::String (t, 2) + (cpu ? "-cpu" : "-gpu") + ".png");
                file.deleteFile();
                if (juce::FileOutputStream out (file); out.openedOk() && juce::PNGImageFormat().writeImageToStream (image, out))
                    written.add (file);
            }
    return written;
}

std::unique_ptr<StartupSplash> makeStartupSplash()
{
#if JUCE_WINDOWS
    if (juce::SystemStats::getEnvironmentVariable ("YW_SPLASH_THREAD", {}) != "0")
        return std::make_unique<ThreadedSplash>();
#endif
    return std::make_unique<SplashScreen>();
}
} // namespace yw
