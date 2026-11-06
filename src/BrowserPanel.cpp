#include "BrowserPanel.h"
#include "LookAndFeel.h"

using namespace yw;

namespace
{
    enum class Kind { folder, file, plugin, info };

    class Item : public juce::TreeViewItem
    {
    public:
        Item (juce::String t, Kind k, juce::File f = {}) : text (std::move (t)), kind (k), file (std::move (f)) {}

        bool mightContainSubItems() override
        {
            if (kind == Kind::folder)
                return getNumSubItems() > 0 || file.isDirectory();
            return false;
        }

        juce::String getUniqueName() const override { return text; }
        int getItemHeight() const override { return 22; }

        void itemOpennessChanged (bool isNowOpen) override
        {
            // Lazy-load real folders the first time they are opened.
            if (isNowOpen && kind == Kind::folder && file.isDirectory() && getNumSubItems() == 0)
            {
                auto children = file.findChildFiles (juce::File::findFilesAndDirectories | juce::File::ignoreHiddenFiles, false);
                children.sort();
                for (auto& c : children)
                    if (c.isDirectory())
                        addSubItem (new Item (c.getFileName(), Kind::folder, c));
                for (auto& c : children)
                    if (! c.isDirectory())
                        addSubItem (new Item (c.getFileName(), Kind::file, c));
            }
        }

        void paintItem (juce::Graphics& g, int w, int h) override
        {
            if (isSelected())
                g.fillAll (theme::accentSoft);

            auto r = juce::Rectangle<int> (0, 0, w, h).reduced (4, 0);
            auto iconArea = r.removeFromLeft (14).toFloat().withSizeKeepingCentre (10.0f, 10.0f);

            switch (kind)
            {
                case Kind::folder: g.setColour (theme::textDim); g.drawRect (iconArea, 1.2f); break;
                case Kind::file:   g.setColour (theme::textDim); g.fillEllipse (iconArea.reduced (2.5f)); break;
                case Kind::plugin: g.setColour (theme::accent);  g.fillRoundedRectangle (iconArea.reduced (1.0f), 2.0f); break;
                case Kind::info:   break;
            }

            r.removeFromLeft (4);
            g.setColour (kind == Kind::info ? theme::textDim : theme::text);
            g.setFont (theme::font (13.0f, kind == Kind::folder && getParentItem() != nullptr
                                                && getParentItem()->getParentItem() == nullptr));
            g.drawText (text, r, juce::Justification::centredLeft, true);
        }

        void itemClicked (const juce::MouseEvent& e) override
        {
            if (kind == Kind::folder && ! e.mods.isPopupMenu())
                setOpen (! isOpen());

            if (e.mods.isPopupMenu() && file.exists())
            {
                juce::PopupMenu m;
                if (kind == Kind::folder)
                {
                    m.addItem (1, "Expand all");
                    m.addItem (2, "Collapse all");
                    m.addSeparator();
                    m.addItem (3, "Open in Explorer");
                }
                else
                {
                    m.addItem (4, "Send to selected channel", false);
                    m.addItem (5, "Open in new channel", false);
                    m.addSeparator();
                    m.addItem (6, "Open parent folder");
                    m.addItem (7, "Locate file");
                }
                m.showMenuAsync (juce::PopupMenu::Options(), [this] (int r)
                {
                    if (r == 1) setOpen (true);
                    else if (r == 2) setOpen (false);
                    else if (r == 3) file.startAsProcess();
                    else if (r == 6) file.getParentDirectory().startAsProcess();
                    else if (r == 7) file.revealToUser();
                });
            }
        }

        juce::var getDragSourceDescription() override
        {
            return file.existsAsFile() ? juce::var (file.getFullPathName()) : juce::var();
        }

        juce::String text;
        Kind kind;
        juce::File file;
    };

    Item* folder (Item* parent, const juce::String& name, juce::File f = {})
    {
        auto* i = new Item (name, Kind::folder, f);
        parent->addSubItem (i);
        return i;
    }
}

BrowserPanel::BrowserPanel (AppState& s) : state (s)
{
    menuButton.setTooltip ("Browser options");
    collapseButton.setTooltip ("Collapse all");
    refreshButton.setTooltip ("Refresh (Ctrl+R)");
    closeButton.setTooltip ("Close browser (Alt+F8)");

    collapseButton.onClick = [this] { if (root) for (int i = 0; i < root->getNumSubItems(); ++i) root->getSubItem (i)->setOpen (false); };
    refreshButton.onClick  = [this] { rebuildTree(); };
    closeButton.onClick    = [this] { if (onClose) onClose(); };

    for (auto* b : { &menuButton, &collapseButton, &refreshButton, &closeButton })
        addAndMakeVisible (b);

    const char* tabNames[] = { "All", "Project", "Plugins", "Starred" };
    for (int i = 0; i < 4; ++i)
    {
        auto* t = tabs.add (new juce::TextButton (tabNames[i]));
        t->setRadioGroupId (101);
        t->setClickingTogglesState (true);
        t->onClick = [this, i] { setTab (i); };
        addAndMakeVisible (t);
    }
    tabs[0]->setToggleState (true, juce::dontSendNotification);

    search.setTextToShowWhenEmpty ("Search", theme::textDim);
    search.onTextChange = [this] { rebuildTree(); };
    search.onEscapeKey  = [this] { search.clear(); rebuildTree(); };
    addAndMakeVisible (search);

    tree.setRootItemVisible (false);
    tree.setDefaultOpenness (false);
    tree.setIndentSize (14);
    addAndMakeVisible (tree);

    state.knownPlugins.addChangeListener (this);
    rebuildTree();
}

BrowserPanel::~BrowserPanel()
{
    state.knownPlugins.removeChangeListener (this);
    tree.setRootItem (nullptr);
}

void BrowserPanel::setTab (int index)
{
    currentTab = index;
    rebuildTree();
}

void BrowserPanel::rebuildTree()
{
    tree.setRootItem (nullptr);
    auto* r = new Item ("root", Kind::folder);
    root.reset (r);

    const auto query = search.getText().trim().toLowerCase();
    auto matches = [&query] (const juce::String& s) { return query.isEmpty() || s.toLowerCase().contains (query); };

    const bool all = currentTab == 0;

    if (all || currentTab == 1)
    {
        auto* proj = folder (r, "Current project");
        for (auto n : { "History", "Patterns", "Generators", "Effects", "Remote control", "Samples" })
            folder (proj, n);
        if (! all) proj->setOpen (true);
    }

    if (all || currentTab == 2)
    {
        auto* lib = folder (r, "Plugin library");
        auto* gens = folder (lib, "Generators");
        auto* fx   = folder (lib, "Effects");

        for (auto& d : state.knownPlugins.getTypes())
            if (matches (d.name) || matches (d.manufacturerName))
                (d.isInstrument ? gens : fx)->addSubItem (new Item (d.name + "  (" + d.pluginFormatName + ")", Kind::plugin));

        if (state.knownPlugins.getNumTypes() == 0)
            lib->addSubItem (new Item ("No plugins yet: Options > Manage plugins", Kind::info));

        if (! all || query.isNotEmpty()) { lib->setOpen (true); gens->setOpen (true); fx->setOpen (true); }
    }

    if (all)
    {
        const auto docs  = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);
        const auto music = juce::File::getSpecialLocation (juce::File::userMusicDirectory);
        const auto data  = docs.getChildFile ("YW Studio");

        for (auto n : { "Channel presets", "Mixer presets", "Plugin presets", "Projects", "Recorded", "Rendered" })
        {
            auto f = data.getChildFile (n);
            f.createDirectory();
            folder (r, n, f);
        }
        if (music.isDirectory()) folder (r, "Music", music);
        folder (r, "Documents", docs);
    }

    if (currentTab == 3)
        r->addSubItem (new Item ("Nothing starred yet", Kind::info));

    tree.setRootItem (root.get());
}

void BrowserPanel::paint (juce::Graphics& g)
{
    g.fillAll (theme::bg);
    g.setColour (theme::bgRaised);
    g.fillRect (getLocalBounds().removeFromTop (28));
    g.setColour (theme::text);
    g.setFont (theme::font (13.0f, true));
    g.drawText ("Browser", getLocalBounds().removeFromTop (28).withTrimmedLeft (30), juce::Justification::centredLeft);
    g.setColour (theme::outline);
    g.drawVerticalLine (getWidth() - 1, 0.0f, (float) getHeight());
}

void BrowserPanel::resized()
{
    auto r = getLocalBounds().withTrimmedRight (1);
    auto header = r.removeFromTop (28).reduced (3);
    menuButton.setBounds (header.removeFromLeft (22));
    closeButton.setBounds (header.removeFromRight (22));
    header.removeFromRight (2);
    refreshButton.setBounds (header.removeFromRight (22));
    header.removeFromRight (2);
    collapseButton.setBounds (header.removeFromRight (22));

    auto tabRow = r.removeFromTop (26).reduced (3, 2);
    const int w = tabRow.getWidth() / tabs.size();
    for (auto* t : tabs) t->setBounds (tabRow.removeFromLeft (w).reduced (1, 0));

    search.setBounds (r.removeFromTop (28).reduced (4, 3));
    tree.setBounds (r.reduced (2));
}
