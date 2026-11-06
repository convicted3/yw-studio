#include <JuceHeader.h>
#include "AppState.h"
#include "LookAndFeel.h"
#include "SplashWindow.h"
#include "MainComponent.h"

// Main window without a native title bar: the top toolbar is the caption (drag, double-click, buttons).
class MainWindow : public juce::DocumentWindow
{
public:
    explicit MainWindow (AppState& state)
        : juce::DocumentWindow ("YW Studio", yw::theme::workspace, 0, false)
    {
        setUsingNativeTitleBar (false);
        setTitleBarHeight (0);
        auto* content = new MainComponent (state);
        setContentOwned (content, true);
        addKeyListener (content);

        setResizable (true, false);
        setResizeLimits (1100, 640, 10000, 10000);
        centreWithSize (getWidth(), getHeight());
        setName ("YW Studio - Untitled");
        // Not added to the desktop yet: it appears only after the splash has finished.
    }

    juce::BorderSize<int> getBorderThickness() const override { return isFullScreen() ? juce::BorderSize<int> (0) : juce::BorderSize<int> (3); }

    void closeButtonPressed() override { juce::JUCEApplication::getInstance()->systemRequestedQuit(); }
};

class YWStudioApplication : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override    { return JUCE_APPLICATION_NAME_STRING; }
    const juce::String getApplicationVersion() override { return JUCE_APPLICATION_VERSION_STRING; }
    bool moreThanOneInstanceAllowed() override          { return false; }

    void initialise (const juce::String& commandLine) override
    {
        juce::LookAndFeel::setDefaultLookAndFeel (&lookAndFeel);

        // Developer tool: render the splash animation to PNG frames and quit.
        if (commandLine.contains ("--splash-frames"))
        {
            dumpSplashFrames (juce::File (commandLine.fromFirstOccurrenceOf ("--splash-frames", false, false).trim().unquoted()));
            quit();
            return;
        }

        state = std::make_unique<AppState>();

        if (state->getShowSplash())
        {
            splash = std::make_unique<SplashWindow>();
            splash->show();
        }

        runStartupStep (0);
    }

    void shutdown() override
    {
        mainWindow.reset();
        splash.reset();
        state.reset();
        juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
    }

    void systemRequestedQuit() override { quit(); }

    void anotherInstanceStarted (const juce::String&) override
    {
        if (mainWindow) mainWindow->toFront (true);
    }

private:
    void runStartupStep (int step)
    {
        struct Step { std::function<void()> fn; };
        const Step steps[] = {
            { [this] { state->loadSettings(); } },
            { [this] { state->initialiseAudio(); } },
            { [this] { state->loadPluginDatabase(); } },
            { [this] { mainWindow = std::make_unique<MainWindow> (*state); } },
        };

        if (step >= (int) std::size (steps))
        {
            if (splash) splash->finish ([this] { showMainWindow(); });
            else        showMainWindow();
            return;
        }

        juce::MessageManager::callAsync ([this, step, fn = steps[step].fn]
        {
            fn();
            juce::MessageManager::callAsync ([this, step] { runStartupStep (step + 1); });
        });
    }

    void showMainWindow()
    {
        splash.reset();
        mainWindow->addToDesktop();
        mainWindow->setVisible (true);
        mainWindow->setFullScreen (true); // maximised, like the classic layout
        mainWindow->toFront (true);
    }

    void dumpSplashFrames (const juce::File& dir)
    {
        dir.createDirectory();
        const int w = 560, h = 420;
        int index = 0;
        for (double ms = 0.0; ms <= SplashWindow::introLengthMs + SplashWindow::exitLengthMs + 40.0; ms += 1000.0 / 30.0)
        {
            juce::Image img (juce::Image::ARGB, w, h, true);
            {
                juce::Graphics g (img);
                g.fillAll (juce::Colour (0xff3a6ea5)); // stand-in desktop colour
                const double exitMs = ms - SplashWindow::introLengthMs;
                SplashWindow::renderFrame (g, { 0.0f, 0.0f, (float) w, (float) h }, ms, exitMs >= 0.0 ? exitMs : -1.0);
            }
            juce::PNGImageFormat png;
            juce::FileOutputStream out (dir.getChildFile ("frame_" + juce::String (index++).paddedLeft ('0', 3) + ".png"));
            if (out.openedOk()) { out.setPosition (0); out.truncate(); png.writeImageToStream (img, out); }
        }
    }

    YWLookAndFeel lookAndFeel;
    std::unique_ptr<AppState> state;
    std::unique_ptr<SplashWindow> splash;
    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION (YWStudioApplication)
