#include "BrowserPanel.h"
#include "LookAndFeel.h"

using namespace yw;

namespace
{
    class Item : public juce::TreeViewItem
    {
    public:
        Item (juce::String t, icons::Id i, juce::File f = {}, bool isFolder = true)
            : text (std::move (t)), icon (i), file (std::move (f)), folder (isFolder) {}

        bool mightContainSubItems() override { return folder && (getNumSubItems() > 0 || file.isDirectory()); }
        juce::String getUniqueName() const override { return text; }
        int getItemHeight() const override { return 23; }

        void itemOpennessChanged (bool open) override
        {
            if (open && folder && file.isDirectory() && getNumSubItems() == 0)
            {
                auto kids = file.findChildFiles (juce::File::findFilesAndDirectories | juce::File::ignoreHiddenFiles, false);
                kids.sort();
                for (auto& k : kids) if (k.isDirectory())   addSubItem (new Item (k.getFileName(), icons::Id::folder, k, true));
                for (auto& k : kids) if (! k.isDirectory()) addSubItem (new Item (k.getFileName(), iconFor (k), k, false));
            }
        }

        static icons::Id iconFor (const juce::File& f)
        {
            const auto ext = f.getFileExtension().toLowerCase();
            if (ext == ".wav" || ext == ".mp3" || ext == ".ogg" || ext == ".flac" || ext == ".aif" || ext == ".aiff") return icons::Id::wave;
            if (ext == ".mid" || ext == ".midi") return icons::Id::pianoRoll;
            if (ext == ".yws" || ext == ".flp") return icons::Id::project;
            return icons::Id::file;
        }

        void paintItem (juce::Graphics& g, int w, int h) override
        {
            if (isSelected()) { g.setColour (juce::Colour (0xff27333a)); g.fillRect (0, 0, w, h); }

            auto r = juce::Rectangle<int> (0, 0, w, h);
            const bool top = getParentItem() != nullptr && getParentItem()->getParentItem() == nullptr;
            icons::draw (g, icon, r.removeFromLeft (20).toFloat().reduced (3.0f), top ? juce::Colour (0xffcfd3d5) : theme::textDim);
            r.removeFromLeft (3);
            g.setColour (folder ? juce::Colour (0xffd2d5d6) : theme::text);
            g.setFont (theme::font (17.0f, top ? theme::Weight::medium : theme::Weight::regular));
            g.drawText (text, r, juce::Justification::centredLeft, true);
        }

        void itemClicked (const juce::MouseEvent& e) override
        {
            if (folder && ! e.mods.isPopupMenu()) setOpen (! isOpen());
            if (! e.mods.isPopupMenu() || ! file.exists()) return;

            juce::PopupMenu m;
            if (folder)
            {
                m.addItem (1, "Expand all");
                m.addItem (2, "Collapse all");
                m.addSeparator();
                m.addItem (3, "Open in new tab", false);
                m.addItem (4, "Change color", false);
                m.addItem (5, "Change icon", false);
                m.addSeparator();
                m.addItem (6, "Open");
            }
            else
            {
                m.addItem (10, "Send to selected channel or focused plugin", false);
                m.addItem (11, "Open in new channel", false);
                m.addSeparator();
                juce::PopupMenu sys;
                sys.addItem (12, "Open parent folder");
                sys.addItem (13, "Locate file");
                m.addSubMenu ("System", sys);
            }
            m.showMenuAsync (juce::PopupMenu::Options(), [this] (int r)
            {
                if (r == 1) setOpen (true);
                else if (r == 2) setOpen (false);
                else if (r == 6) file.startAsProcess();
                else if (r == 12) file.getParentDirectory().startAsProcess();
                else if (r == 13) file.revealToUser();
            });
        }

        juce::var getDragSourceDescription() override
        {
            return file.existsAsFile() ? juce::var (file.getFullPathName()) : juce::var();
        }

        juce::String text;
        icons::Id icon;
        juce::File file;
        bool folder;
    };
}

BrowserPanel::BrowserPanel (AppState& s) : state (s)
{
    menuButton.onClick = [this]
    {
        juce::PopupMenu m;
        m.addItem (1, "Search...  Ctrl+F");
        m.addItem (2, "Refresh  Ctrl+R");
        m.addItem (3, "Collapse all");
        m.addSeparator();
        m.addItem (4, "Close browser  Alt+F8");
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&menuButton), [this] (int r)
        {
            if (r == 1) focusSearch();
            else if (r == 2) rebuildTree();
            else if (r == 3) upButton.triggerClick();
            else if (r == 4 && onClose) onClose();
        });
    };
    upButton.onClick   = [this] { if (root) for (int i = 0; i < root->getNumSubItems(); ++i) root->getSubItem (i)->setOpen (false); };
    backButton.onClick = [this] { rebuildTree(); };
    for (auto* b : { &menuButton, &upButton, &backButton }) addAndMakeVisible (b);

    struct T { icons::Id icon; const char* hint; };
    const T defs[] = {
        { icons::Id::browser, "All" },
        { icons::Id::project, "Current project" },
        { icons::Id::speaker, "Plugin database" },
        { icons::Id::globe,   "YW plugin catalog" },
        { icons::Id::cloud,   "Online content" },
        { icons::Id::star,    "Starred" },
        { icons::Id::folder,  "User folders" } };
    for (int i = 0; i < (int) std::size (defs); ++i)
    {
        auto* t = tabs.add (new ToolButton (defs[i].icon, defs[i].hint, ToolButton::Style::flat));
        t->setRadioGroupId (77);
        t->setClickingTogglesState (true);
        t->onClick = [this, i] { setTab (i); };
        addAndMakeVisible (t);
    }
    tabs[0]->setToggleState (true, juce::dontSendNotification);

    tagsSearch.onClick = [this] { focusSearch(); };
    tagsFolder.onClick = [this] { userDataFolder().startAsProcess(); };
    addAndMakeVisible (tagsFolder);
    addAndMakeVisible (tagsSearch);

    search.setTextToShowWhenEmpty ("Search", theme::textDim);
    search.setFont (theme::font (16.0f));
    search.onTextChange = [this] { rebuildTree(); };
    search.onEscapeKey  = [this] { search.clear(); searchVisible = false; resized(); rebuildTree(); };
    addChildComponent (search);

    tree.setRootItemVisible (false);
    tree.setDefaultOpenness (false);
    tree.setIndentSize (16);
    addAndMakeVisible (tree);

    state.knownPlugins.addChangeListener (this);
    rebuildTree();
}

BrowserPanel::~BrowserPanel()
{
    state.knownPlugins.removeChangeListener (this);
    tree.setRootItem (nullptr);
}

juce::File BrowserPanel::userDataFolder() const
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("YW Studio");
}

void BrowserPanel::focusSearch()
{
    searchVisible = true;
    resized();
    search.grabKeyboardFocus();
}

void BrowserPanel::showPluginDatabase()
{
    tabs[2]->setToggleState (true, juce::dontSendNotification);
    setTab (2);
}

void BrowserPanel::setTab (int index)
{
    currentTab = index;
    rebuildTree();
}

void BrowserPanel::rebuildTree()
{
    tree.setRootItem (nullptr);
    root = std::make_unique<Item> ("root", icons::Id::none);

    const auto q = search.getText().trim().toLowerCase();
    auto matches = [&q] (const juce::String& s) { return q.isEmpty() || s.toLowerCase().contains (q); };
    const auto data = userDataFolder();

    auto add = [this] (juce::TreeViewItem* parent, const juce::String& name, icons::Id icon, juce::File f = {}) -> Item*
    {
        auto* i = new Item (name, icon, f, true);
        (parent ? parent : root.get())->addSubItem (i);
        return i;
    };

    auto addPluginDatabase = [&] (bool open)
    {
        auto* db = add (nullptr, "Plugin database", icons::Id::speaker);
        auto* inst = add (db, "Installed", icons::Id::folder);
        auto* gens = add (inst, "Generators", icons::Id::keys);
        auto* fx   = add (inst, "Effects", icons::Id::wave);
        for (auto& d : state.knownPlugins.getTypes())
            if (matches (d.name) || matches (d.manufacturerName))
                (d.isInstrument ? gens : fx)->addSubItem (new Item (d.name, icons::Id::plug, {}, false));
        if (state.knownPlugins.getNumTypes() == 0)
            inst->addSubItem (new Item ("Scan plugins: Options > Manage plugins", icons::Id::none, {}, false));
        if (open || q.isNotEmpty()) { db->setOpen (true); inst->setOpen (true); gens->setOpen (true); fx->setOpen (true); }
    };

    auto addFolder = [&] (const juce::String& name, icons::Id icon)
    {
        auto f = data.getChildFile (name);
        f.createDirectory();
        if (matches (name)) add (nullptr, name, icon, f);
    };

    switch (currentTab)
    {
        case 0: // All
        {
            auto* cp = add (nullptr, "Current project", icons::Id::project);
            for (auto n : { "History", "Patterns", "Effects", "Generators", "Remote control", "Initialized controls", "Samples" })
                add (cp, n, icons::Id::folder);
            add (nullptr, "Recent files", icons::Id::undo);
            addPluginDatabase (false);
            addFolder ("Plugin presets", icons::Id::speaker);
            addFolder ("Channel presets", icons::Id::preset);
            addFolder ("Mixer presets", icons::Id::mixer);
            addFolder ("Scores", icons::Id::pianoRoll);
            addFolder ("Backup", icons::Id::undo);
            for (auto n : { "Clipboard files", "Demo projects", "Envelopes", "Impulses", "Misc", "My projects" })
                addFolder (n, icons::Id::folder);
            addFolder ("Packs", icons::Id::sampler);
            addFolder ("Recorded", icons::Id::wave);
            addFolder ("Rendered", icons::Id::wave);
            addFolder ("Sliced audio", icons::Id::slice);
            addFolder ("Soundfonts", icons::Id::folder);
            addFolder ("Templates", icons::Id::folder);
            add (nullptr, "User data", icons::Id::star, data);
            break;
        }
        case 1:
        {
            auto* cp = add (nullptr, "Current project", icons::Id::project);
            for (auto n : { "History", "Patterns", "Effects", "Generators", "Remote control", "Initialized controls", "Samples" })
                add (cp, n, icons::Id::folder);
            cp->setOpen (true);
            break;
        }
        case 2: addPluginDatabase (true); break;
        case 3: root->addSubItem (new Item ("Plugin catalog - coming soon", icons::Id::globe, {}, false)); break;
        case 4: root->addSubItem (new Item ("Online content - coming soon", icons::Id::cloud, {}, false)); break;
        case 5: root->addSubItem (new Item ("Nothing starred yet", icons::Id::star, {}, false)); break;
        case 6:
        {
            add (nullptr, "User data", icons::Id::star, data);
            const auto music = juce::File::getSpecialLocation (juce::File::userMusicDirectory);
            if (music.isDirectory()) add (nullptr, "Music", icons::Id::folder, music);
            add (nullptr, "Documents", icons::Id::folder, juce::File::getSpecialLocation (juce::File::userDocumentsDirectory));
            add (nullptr, "Desktop", icons::Id::folder, juce::File::getSpecialLocation (juce::File::userDesktopDirectory));
            break;
        }
        default: break;
    }

    tree.setRootItem (root.get());
}

void BrowserPanel::paint (juce::Graphics& g)
{
    auto r = getLocalBounds();

    auto header = r.removeFromTop (22);
    drawPanelHeader (g, header, {}, icons::Id::none);
    g.setColour (theme::textDim);
    g.setFont (theme::font (15.0f));
    g.drawText ("Browser", header.withTrimmedLeft (64), juce::Justification::centredLeft);

    auto tabRow = r.removeFromTop (30);
    g.setColour (theme::browserTabs);
    g.fillRect (tabRow);
    // selected tab gets a framed highlight
    for (auto* t : tabs)
        if (t->getToggleState())
        {
            g.setColour (juce::Colour (0xff2c363c));
            g.fillRoundedRectangle (t->getBounds().toFloat().reduced (1.0f), 3.0f);
            g.setColour (juce::Colour (0xff5d6a71));
            g.drawRoundedRectangle (t->getBounds().toFloat().reduced (1.0f), 3.0f, 1.0f);
        }

    auto tags = r.removeFromBottom (26);
    g.setColour (theme::tagsBar);
    g.fillRect (tags);
    g.setColour (theme::text);
    g.setFont (theme::font (14.0f, theme::Weight::semibold));
    g.drawText ("TAGS", tags.withTrimmedLeft (10), juce::Justification::centredLeft);

    if (searchVisible) r.removeFromTop (28);
    g.setGradientFill (juce::ColourGradient (theme::browserBg, 0, (float) r.getY(), theme::browserBg2, 0, (float) r.getBottom(), false));
    g.fillRect (r);

    g.setColour (juce::Colours::black);
    g.drawVerticalLine (getWidth() - 1, 0.0f, (float) getHeight());
}

void BrowserPanel::resized()
{
    auto r = getLocalBounds().withTrimmedRight (1);
    auto header = r.removeFromTop (22);
    menuButton.setBounds (header.removeFromLeft (20));
    upButton.setBounds (header.removeFromLeft (20));
    backButton.setBounds (header.removeFromLeft (20));

    auto tabRow = r.removeFromTop (30).reduced (4, 2);
    const int w = tabRow.getWidth() / tabs.size();
    for (auto* t : tabs) t->setBounds (tabRow.removeFromLeft (w).reduced (1, 0));

    auto tags = r.removeFromBottom (26);
    tagsSearch.setBounds (tags.removeFromRight (24).reduced (2));
    tagsFolder.setBounds (tags.removeFromRight (24).reduced (2));

    search.setVisible (searchVisible);
    if (searchVisible) search.setBounds (r.removeFromTop (28).reduced (4, 3));
    tree.setBounds (r.reduced (4, 4));
}
