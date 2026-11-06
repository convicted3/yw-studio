#include "SettingsWindow.h"
#include "LookAndFeel.h"
#include "PanelWindow.h"

using namespace yw;

namespace
{
    class GeneralPage : public juce::Component
    {
    public:
        explicit GeneralPage (AppState& s) : state (s)
        {
            splash.setButtonText ("Show startup splash screen");
            splash.setToggleState (state.getShowSplash(), juce::dontSendNotification);
            splash.onClick = [this] { state.setShowSplash (splash.getToggleState()); state.saveSettings(); };
            addAndMakeVisible (splash);

            language.addItem ("English", 1);
            language.addItem (juce::String::fromUTF8 ("\xd0\xa0\xd1\x83\xd1\x81\xd1\x81\xd0\xba\xd0\xb8\xd0\xb9 (coming soon)"), 2);
            language.setItemEnabled (2, false);
            language.setSelectedId (1, juce::dontSendNotification);
            addAndMakeVisible (language);

            languageLabel.setText ("Language", juce::dontSendNotification);
            addAndMakeVisible (languageLabel);
        }

        void paint (juce::Graphics& g) override
        {
            g.fillAll (theme::bg);
            g.setColour (theme::textDim);
            g.setFont (theme::font (12.0f, true));
            g.drawText ("MISCELLANEOUS", 16, 12, 300, 18, juce::Justification::centredLeft);
            g.drawText ("LANGUAGE", 16, 84, 300, 18, juce::Justification::centredLeft);
        }

        void resized() override
        {
            splash.setBounds (16, 34, 320, 24);
            languageLabel.setBounds (16, 106, 100, 24);
            language.setBounds (120, 106, 220, 24);
        }

    private:
        AppState& state;
        juce::ToggleButton splash;
        juce::ComboBox language;
        juce::Label languageLabel;
    };
}

SettingsWindow::SettingsWindow (AppState& s)
    : juce::DocumentWindow ("Settings", theme::bg, juce::DocumentWindow::closeButton), state (s)
{
    setUsingNativeTitleBar (true);

    auto* t = new juce::TabbedComponent (juce::TabbedButtonBar::TabsAtTop);
    tabs = t;
    t->setTabBarDepth (30);

    auto* audioPage = new juce::AudioDeviceSelectorComponent (s.deviceManager, 0, 2, 2, 2,
                                                              false, false, true, false);

    auto* midiPage = new juce::AudioDeviceSelectorComponent (s.deviceManager, 0, 0, 0, 0,
                                                             true, true, false, true);

    t->addTab ("MIDI",    theme::bg, midiPage, true);
    t->addTab ("Audio",   theme::bg, audioPage, true);
    t->addTab ("General", theme::bg, new GeneralPage (s), true);
    t->addTab ("File",    theme::bg, new ComingSoonView ("File settings"), true);
    t->addTab ("Project", theme::bg, new ComingSoonView ("Project settings"), true);

    t->setSize (640, 560);
    setContentOwned (t, true);
    setResizable (true, false);
    centreWithSize (getWidth(), getHeight());
}

void SettingsWindow::showTab (Tab tab)
{
    if (tabs != nullptr) tabs->setCurrentTabIndex ((int) tab);
    setVisible (true);
    toFront (true);
}

void SettingsWindow::closeButtonPressed()
{
    state.saveSettings();
    setVisible (false);
}
