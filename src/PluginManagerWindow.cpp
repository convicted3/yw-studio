#include "PluginManagerWindow.h"
#include "LookAndFeel.h"

using namespace yw;

class PluginManagerWindow::Content : public juce::Component
{
public:
    explicit Content (AppState& s)
        : state (s),
          list (s.formatManager, s.knownPlugins, s.getDeadMansPedalFile(), &s.settings(), true)
    {
        info.setText ("Plugins are scanned from the standard folders for each format. "
                      "Use \"Options\" below to scan, add search folders or remove entries. "
                      "Plugins that crash during a scan are blacklisted automatically.",
                      juce::dontSendNotification);
        info.setColour (juce::Label::textColourId, theme::textDim);
        info.setFont (theme::font (16.0f));
        info.setJustificationType (juce::Justification::topLeft);
        addAndMakeVisible (info);

        formats.setText ("Formats: " + formatNames(), juce::dontSendNotification);
        formats.setFont (theme::bold (13.0f));
        addAndMakeVisible (formats);

        addAndMakeVisible (list);
        setSize (820, 560);
    }

    void paint (juce::Graphics& g) override { g.fillAll (theme::panelBody); }

    void resized() override
    {
        auto r = getLocalBounds().reduced (10);
        formats.setBounds (r.removeFromTop (22));
        info.setBounds (r.removeFromTop (40));
        r.removeFromTop (6);
        list.setBounds (r);
    }

private:
    juce::String formatNames() const
    {
        juce::StringArray names;
        for (auto* f : state.formatManager.getFormats())
            names.add (f->getName());
        return names.joinIntoString (", ");
    }

    AppState& state;
    juce::Label info, formats;
    juce::PluginListComponent list;
};

PluginManagerWindow::PluginManagerWindow (AppState& s)
    : juce::DocumentWindow ("Plugin Manager", theme::panelBody, juce::DocumentWindow::closeButton)
{
    setUsingNativeTitleBar (true);
    setContentOwned (new Content (s), true);
    setResizable (true, false);
    setResizeLimits (560, 360, 4000, 3000);
    centreWithSize (getWidth(), getHeight());
}
