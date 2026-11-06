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

        setupLogging();
        log ("YW Studio " + getApplicationVersion() + " starting on " + juce::SystemStats::getOperatingSystemName());

        state = std::make_unique<AppState>();

        if (state->getShowSplash())
        {
            // play the intro first, then do the (blocking) startup work while the logo holds still
            splash = std::make_unique<SplashWindow>();
            splash->show ([this] { runStartupStep (0); });
        }
        else
        {
            runStartupStep (0);
        }
    }

    void shutdown() override
    {
        log ("shutting down");
        mainWindow.reset();
        splash.reset();
        state.reset();
        juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
        juce::Logger::setCurrentLogger (nullptr);
    }

    void systemRequestedQuit() override { quit(); }

    void anotherInstanceStarted (const juce::String&) override
    {
        if (mainWindow) mainWindow->toFront (true);
    }

private:
    void runStartupStep (int step)
    {
        struct Step { const char* name; std::function<void()> fn; };
        const Step steps[] = {
            { "load settings",        [this] { state->loadSettings(); } },
            { "initialise audio",     [this] { state->initialiseAudio(); } },
            { "load plugin database", [this] { state->loadPluginDatabase(); } },
            { "create main window",   [this] { mainWindow = std::make_unique<MainWindow> (*state); } },
        };

        if (step >= (int) std::size (steps))
        {
            log ("startup complete");
            if (splash) splash->finish ([this] { showMainWindow(); });
            else        showMainWindow();
            return;
        }

        juce::MessageManager::callAsync ([this, step, name = juce::String (steps[step].name), fn = steps[step].fn]
        {
            log ("step: " + name);
            fn();
            juce::MessageManager::callAsync ([this, step] { runStartupStep (step + 1); });
        });
    }

    void showMainWindow()
    {
        log ("showing main window");
        splash.reset();
        mainWindow->addToDesktop();
        mainWindow->setVisible (true);
        mainWindow->setFullScreen (true); // maximised, like the classic layout
        mainWindow->toFront (true);
        log ("main window visible");
    }

    //==============================================================================
    // Log file and crash report in %APPDATA%\\YW Studio, so problems on user machines can be diagnosed.
    static juce::File appDataFolder()
    {
        auto f = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("YW Studio");
        f.createDirectory();
        return f;
    }

    static void crashHandler (void*)
    {
        auto report = appDataFolder().getChildFile ("crash.log");
        report.appendText ("\n=== Crash " + juce::Time::getCurrentTime().toString (true, true) + " ===\n"
                           + juce::SystemStats::getStackBacktrace() + "\n");
    }

    void setupLogging()
    {
        juce::SystemStats::setApplicationCrashHandler (crashHandler);
        auto file = appDataFolder().getChildFile ("yw-studio.log");
        file.deleteFile();
        logger.reset (new juce::FileLogger (file, "YW Studio log", 0));
        juce::Logger::setCurrentLogger (logger.get());
    }

    static void log (const juce::String& s) { juce::Logger::writeToLog (s); }

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
    std::unique_ptr<juce::FileLogger> logger;
};

START_JUCE_APPLICATION (YWStudioApplication)
