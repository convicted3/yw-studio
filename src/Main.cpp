#include <JuceHeader.h>
#include "AppState.h"
#include "LookAndFeel.h"
#include "SplashWindow.h"
#include "MainComponent.h"

class MainWindow : public juce::DocumentWindow
{
public:
    explicit MainWindow (AppState& state)
        : juce::DocumentWindow ("YW Studio", yw::theme::bgDeep, juce::DocumentWindow::allButtons, false)
    {
        setUsingNativeTitleBar (true);
        auto* content = new MainComponent (state);
        setContentOwned (content, true);
        addKeyListener (content);

        setResizable (true, false);
        setResizeLimits (960, 600, 10000, 10000);
        centreWithSize (getWidth(), getHeight());
        setName ("YW Studio - Untitled");
        // Not added to the desktop yet: it appears only after the splash closes.
    }

    void closeButtonPressed() override
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }
};

class YWStudioApplication : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override    { return JUCE_APPLICATION_NAME_STRING; }
    const juce::String getApplicationVersion() override { return JUCE_APPLICATION_VERSION_STRING; }
    bool moreThanOneInstanceAllowed() override          { return false; }

    void initialise (const juce::String&) override
    {
        juce::LookAndFeel::setDefaultLookAndFeel (&lookAndFeel);
        state = std::make_unique<AppState>();

        startTime = juce::Time::getMillisecondCounter();

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
    // Each startup step runs on its own message-loop iteration so the splash can repaint.
    void runStartupStep (int step)
    {
        struct Step { const char* status; std::function<void()> fn; };
        const Step steps[] = {
            { "Loading settings...",        [this] { state->loadSettings(); } },
            { "Initializing audio device...", [this] { state->initialiseAudio(); } },
            { "Loading plugin database...", [this] { state->loadPluginDatabase(); } },
            { "Building interface...",      [this] { mainWindow = std::make_unique<MainWindow> (*state); } },
        };
        const int numSteps = (int) std::size (steps);

        if (step >= numSteps)
        {
            finishStartup();
            return;
        }

        if (splash)
        {
            splash->setStatus (steps[step].status);
            splash->setProgress ((float) step / (float) numSteps);
        }

        juce::MessageManager::callAsync ([this, step, fn = steps[step].fn]
        {
            fn();
            juce::MessageManager::callAsync ([this, step] { runStartupStep (step + 1); });
        });
    }

    void finishStartup()
    {
        if (splash) { splash->setStatus ("Ready"); splash->setProgress (1.0f); }

        // Keep the splash visible for at least ~1.2 s so it doesn't just flash.
        const auto elapsed = (int) (juce::Time::getMillisecondCounter() - startTime);
        const int wait = splash ? juce::jmax (0, 1200 - elapsed) : 0;

        juce::Timer::callAfterDelay (wait, [this]
        {
            splash.reset();
            mainWindow->addToDesktop();
            mainWindow->setVisible (true);
            mainWindow->setFullScreen (true); // maximised, like FL
            mainWindow->toFront (true);
        });
    }

    YWLookAndFeel lookAndFeel;
    std::unique_ptr<AppState> state;
    std::unique_ptr<SplashWindow> splash;
    std::unique_ptr<MainWindow> mainWindow;
    juce::uint32 startTime = 0;
};

START_JUCE_APPLICATION (YWStudioApplication)
