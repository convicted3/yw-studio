// Round 190ba (issue #104): the Plugin database as a folder the user can arrange in the file manager
// (core/PluginDbFolder.h).

#include "core/PluginDbFolder.h"

namespace plugindb190ba
{
namespace fs = std::filesystem;
inline fs::path u8 (const std::string& s) { return fs::path ((const char8_t*) s.c_str()); }
inline std::string str (const fs::path& p)
{
    const auto s = p.generic_u8string();
    return std::string ((const char*) s.data(), s.size());
}
struct TempRoot
{
    fs::path dir;
    std::string root;
    TempRoot()
    {
        static int n = 0;
        dir = fs::temp_directory_path() / ("yw-plugindb190ba-" + std::to_string ((long long) std::chrono::steady_clock::now().time_since_epoch().count()) + "-" + std::to_string (++n));
        fs::create_directories (dir);
        root = str (dir / "Plugin database");
    }
    ~TempRoot()
    {
        std::error_code ec;
        fs::remove_all (dir, ec);
    }
    fs::path fx (const std::string& rel) const { return u8 (root) / "Effects" / u8 (rel); }
    fs::path gen (const std::string& rel) const { return u8 (root) / "Generators" / u8 (rel); }
    bool has (const fs::path& p) const { return fs::exists (p); }
};
inline std::vector<yw::plugindb::Plugin> plugins()
{
    using P = yw::plugindb::Plugin;
    return {
        P { "yw:fx:Fruity Limiter", "Fruity Limiter", "FL engines", false, true, "Dynamics" },
        P { "yw:fx:Maximus", "Maximus", "FL engines", false, true, "Dynamics" },
        P { "yw:fx:Facet EQ", "Facet EQ", "YW", false, true, "YW Studio" },
        P { "yw:gen:Sytrus", "Sytrus", "FL engines", true, true, "Synth classic" },
        P { "VST3-ProQ-1234", "Pro-Q 3", "VST3", false, false, "" },
        P { "VST-ProQ-99", "Pro-Q 3", "VST", false, false, "" },
        P { "CLAP-synth-1", "Vital", "CLAP", true, false, "" },
    };
}
inline int entryFiles (const fs::path& dir)
{
    int n = 0;
    std::error_code ec;
    for (fs::recursive_directory_iterator it (dir, ec), end; it != end && ! ec; it.increment (ec))
        n += it->path().extension() == yw::plugindb::entryExtension ? 1 : 0;
    return n;
}
} // namespace plugindb190ba

TEST (plugindb190ba_safe_file_names)
{
    using yw::plugindb::safeFileName;
    CHECK (safeFileName ("Pro-Q 3") == "Pro-Q 3");
    CHECK (safeFileName ("A/B\\C:D*E?F\"G<H>I|J") == "A_B_C_D_E_F_G_H_I_J");
    CHECK (safeFileName ("  name. . ") == "name");
    CHECK (safeFileName ("") == "Plugin" && safeFileName (" .. ") == "Plugin");
    CHECK (safeFileName ("CON") == "_CON" && safeFileName ("nul.txt") == "_nul.txt" && safeFileName ("COM1") == "_COM1" && safeFileName ("Console") == "Console");
    CHECK (safeFileName ("tab\there") == "tab_here");
    CHECK (safeFileName ("Ré\xe2\x80\xa2verb \xe6\x97\xa5\xe6\x9c\xac") == "Ré\xe2\x80\xa2verb \xe6\x97\xa5\xe6\x9c\xac"); // UTF-8 kept
    std::string longName;
    for (int i = 0; i < 100; ++i)
        longName += "\xc3\xa9"; // 200 bytes of "é"
    const auto cut = safeFileName (longName);
    CHECK (cut.size() == 120 && ((unsigned char) cut.back() & 0xc0) == 0x80); // cut between characters
}

TEST (plugindb190ba_entry_round_trip)
{
    yw::plugindb::Entry e;
    e.key = "VST3-Some=Plugin-1";
    e.name = "Some = Plugin";
    e.format = "VST3";
    e.file = "Some = Plugin";
    e.shown = "1_EQ";
    e.instrument = true;
    yw::plugindb::Entry back;
    CHECK (yw::plugindb::parseEntry (yw::plugindb::formatEntry (e), back));
    CHECK (back.key == e.key && back.name == e.name && back.format == e.format && back.file == e.file && back.shown == e.shown && back.instrument);
    CHECK (yw::plugindb::parseEntry ("\xef\xbb\xbfYW plugin 1\r\nkey=k\r\nkind=effect\r\n", back) && back.key == "k" && ! back.instrument);
    CHECK (! yw::plugindb::parseEntry ("something else\nkey=k\n", back));
    CHECK (! yw::plugindb::parseEntry ("YW plugin 1\nname=no key\n", back));
}

TEST (plugindb190ba_first_run_makes_the_tree_from_the_settings)
{
    using namespace plugindb190ba;
    TempRoot t;
    yw::plugindb::State state;
    // what YW kept until now: a folder with a subfolder, a built-in and a scanned plugin filed, a renamed plugin
    yw::plugincatalog::moveToFolder (state.folders, false, "yw:fx:Maximus", "Stock/Dynamics");
    yw::plugincatalog::moveToFolder (state.folders, false, "VST3-ProQ-1234", "Third party");
    yw::plugincatalog::addFolder (state.folders, true, "Empty one");
    state.names["yw:fx:Fruity Limiter"] = "1_Limiter";
    state.folders.view = 2;
    const auto wanted = state;

    const auto r = yw::plugindb::refresh (t.root, plugins(), state);
    CHECK (r.ok && r.migrated && r.created == 5); // 4 built-in ones + the filed VST3
    CHECK (t.has (t.fx ("Stock/Dynamics/Maximus.ywplugin")));
    CHECK (t.has (t.fx ("Third party/Pro-Q 3.ywplugin")));
    CHECK (t.has (t.fx ("Dynamics/1_Limiter.ywplugin"))); // the renamed one under its name, in its default folder
    CHECK (t.has (t.fx ("YW Studio/Facet EQ.ywplugin")));
    CHECK (t.has (t.gen ("Synth classic/Sytrus.ywplugin")));
    CHECK (fs::is_directory (t.gen ("Empty one")));
    CHECK (t.has (u8 (t.root) / "database.index") && t.has (u8 (t.root) / "Read me.txt"));
    // unfiled scanned plugins have no entry in the arrangement, but every plugin has one under Installed
    CHECK (entryFiles (u8 (t.root) / "Effects") == 4 && entryFiles (u8 (t.root) / "Generators") == 1);
    CHECK (t.has (u8 (t.root) / "Installed/Effects/VST3/Pro-Q 3.ywplugin") && t.has (u8 (t.root) / "Installed/Effects/VST/Pro-Q 3.ywplugin"));
    CHECK (t.has (u8 (t.root) / "Installed/Generators/CLAP/Vital.ywplugin") && t.has (u8 (t.root) / "Installed/Effects/YW/Facet EQ.ywplugin"));
    CHECK (entryFiles (u8 (t.root) / "Installed") == 7);
    // the model is what it was (default folders are not the user's)
    CHECK (state == wanted && state.folders.view == 2);
    CHECK (state.folders.effects == (std::set<std::string> { "Stock", "Stock/Dynamics", "Third party" }));

    // again: nothing changes
    const auto again = yw::plugindb::refresh (t.root, plugins(), state);
    CHECK (again.ok && ! again.migrated && again.created == 0 && again.moved == 0 && again.removed == 0 && state == wanted);
    CHECK (entryFiles (u8 (t.root)) == 12);
}

TEST (plugindb190ba_changes_made_in_the_file_manager_are_respected)
{
    using namespace plugindb190ba;
    TempRoot t;
    yw::plugindb::State state;
    yw::plugindb::refresh (t.root, plugins(), state);
    CHECK (state.folders.of.empty() && state.folders.effects.empty() && state.names.empty());

    // a new folder with a subfolder, a built-in plugin moved into it
    fs::create_directories (t.fx ("Image-Line/Dynamics"));
    fs::rename (t.fx ("Dynamics/Maximus.ywplugin"), t.fx ("Image-Line/Dynamics/Maximus.ywplugin"));
    // a scanned plugin copied from Installed into a new folder (the way to file one), in a folder named with Unicode
    fs::create_directories (t.fx ("3rd Party/\xe6\x97\xa5\xe6\x9c\xac EQ"));
    fs::copy_file (u8 (t.root) / "Installed/Effects/VST3/Pro-Q 3.ywplugin", t.fx ("3rd Party/\xe6\x97\xa5\xe6\x9c\xac EQ/Pro-Q 3.ywplugin"));
    // a rename, a delete, an empty folder
    fs::rename (t.fx ("Dynamics/Fruity Limiter.ywplugin"), t.fx ("Dynamics/1_Limiter.ywplugin"));
    fs::remove (t.fx ("YW Studio/Facet EQ.ywplugin"));
    fs::create_directories (t.gen ("Later"));

    auto r = yw::plugindb::refresh (t.root, plugins(), state);
    CHECK (r.ok && r.created == 0); // the deleted one is not made again
    CHECK (state.folders.of.at ("yw:fx:Maximus") == "Image-Line/Dynamics");
    CHECK (state.folders.of.at ("VST3-ProQ-1234") == "3rd Party/\xe6\x97\xa5\xe6\x9c\xac EQ");
    CHECK (state.folders.effects == (std::set<std::string> { "3rd Party", "3rd Party/\xe6\x97\xa5\xe6\x9c\xac EQ", "Image-Line", "Image-Line/Dynamics" }));
    CHECK (state.folders.generators == (std::set<std::string> { "Later" }));
    CHECK (state.names.at ("yw:fx:Fruity Limiter") == "1_Limiter" && state.names.size() == 1);
    CHECK (state.folders.hidden == (std::set<std::string> { "yw:fx:Facet EQ" }));
    CHECK (! state.folders.of.count ("yw:fx:Fruity Limiter")); // still in its default folder

    // the menus: the deleted one has no folder, the others sit where the tree says
    std::vector<yw::plugincatalog::BuiltinPlugin> builtins { { "yw:fx:Fruity Limiter", "Fruity Limiter", false, true }, { "yw:fx:Maximus", "Maximus", false, true },
                                                               { "yw:fx:Facet EQ", "Facet EQ", false, false } };
    const auto menus = yw::plugincatalog::withDefaultCategories (state.folders, false, builtins);
    CHECK (menus.of.at ("yw:fx:Fruity Limiter") == "Dynamics" && menus.of.at ("yw:fx:Maximus") == "Image-Line/Dynamics" && ! menus.of.count ("yw:fx:Facet EQ"));
    const auto tree = yw::plugincatalog::menuTree (menus, false, { "yw:fx:Fruity Limiter", "yw:fx:Maximus", "yw:fx:Facet EQ", "VST3-ProQ-1234" });
    CHECK (tree.size() == 3 && tree[0].name == "3rd Party" && tree[1].name == "Dynamics" && tree[2].name == "Image-Line");
    CHECK (tree[2].sub.size() == 1 && tree[2].sub[0].items == std::vector<int> { 1 });

    // the entry copied back from Installed brings the plugin back; a folder renamed in the file manager is followed
    fs::copy_file (u8 (t.root) / "Installed/Effects/YW/Facet EQ.ywplugin", t.fx ("Image-Line/Facet EQ.ywplugin"));
    fs::rename (t.fx ("3rd Party"), t.fx ("Third"));
    yw::plugindb::refresh (t.root, plugins(), state);
    CHECK (state.folders.hidden.empty() && state.folders.of.at ("yw:fx:Facet EQ") == "Image-Line");
    CHECK (state.folders.of.at ("VST3-ProQ-1234") == "Third/\xe6\x97\xa5\xe6\x9c\xac EQ" && ! state.folders.effects.count ("3rd Party"));

    // a whole folder deleted: its built-in plugins leave the menus, the scanned one is simply unfiled
    fs::remove_all (t.fx ("Third"));
    fs::remove_all (t.fx ("Image-Line"));
    yw::plugindb::refresh (t.root, plugins(), state);
    CHECK (state.folders.effects.empty() && ! state.folders.of.count ("VST3-ProQ-1234"));
    CHECK (state.folders.hidden == (std::set<std::string> { "yw:fx:Facet EQ", "yw:fx:Maximus" }));

    // a plugin that is new to YW gets its entry in its default folder; one that left keeps its file
    auto more = plugins();
    more.push_back ({ "yw:fx:New One", "New One", "YW", false, true, "YW Studio" });
    more.erase (more.begin()); // Fruity Limiter (e.g. the FL engines' data is not there now)
    r = yw::plugindb::refresh (t.root, more, state);
    CHECK (r.created == 1 && t.has (t.fx ("YW Studio/New One.ywplugin")) && t.has (t.fx ("Dynamics/1_Limiter.ywplugin")));
    CHECK (! t.has (u8 (t.root) / "Installed/Effects/FL engines/Fruity Limiter.ywplugin") && t.has (u8 (t.root) / "Installed/Effects/YW/New One.ywplugin"));
    yw::plugindb::refresh (t.root, plugins(), state);
    CHECK (state.names.at ("yw:fx:Fruity Limiter") == "1_Limiter");
}

TEST (plugindb190ba_changes_made_in_yw_go_to_the_tree)
{
    using namespace plugindb190ba;
    TempRoot t;
    const auto all = plugins();
    yw::plugindb::State state;
    yw::plugindb::refresh (t.root, all, state);
    auto edit = [&] (std::function<void (yw::plugindb::State&)> change) {
        change (state);
        const auto r = yw::plugindb::apply (t.root, all, state);
        yw::plugindb::refresh (t.root, all, state);
        return r;
    };

    // Move to folder (a new one), for a built-in and for both builds of a scanned plugin with one name
    auto r = edit ([] (yw::plugindb::State& s) {
        yw::plugincatalog::moveToFolder (s.folders, false, "yw:fx:Maximus", "Mix/Bus");
        yw::plugincatalog::moveToFolder (s.folders, false, "VST3-ProQ-1234", "Mix/Bus");
        yw::plugincatalog::moveToFolder (s.folders, false, "VST-ProQ-99", "Mix/Bus");
    });
    CHECK (r.ok && r.moved == 1 && r.created == 2);
    CHECK (t.has (t.fx ("Mix/Bus/Maximus.ywplugin")) && ! t.has (t.fx ("Dynamics/Maximus.ywplugin")));
    // two plugins of one name in one folder: the second one's file carries its format, and that is not a rename
    CHECK (t.has (t.fx ("Mix/Bus/Pro-Q 3.ywplugin")) && t.has (t.fx ("Mix/Bus/Pro-Q 3 (VST).ywplugin")));
    CHECK (state.names.empty() && state.folders.of.at ("VST3-ProQ-1234") == "Mix/Bus" && state.folders.of.at ("VST-ProQ-99") == "Mix/Bus");

    // Rename... and Original name
    r = edit ([] (yw::plugindb::State& s) { s.names["yw:fx:Maximus"] = "2_Max: bus?"; });
    CHECK (r.renamed == 1 && t.has (t.fx ("Mix/Bus/2_Max_ bus_.ywplugin")) && ! t.has (t.fx ("Mix/Bus/Maximus.ywplugin")));
    CHECK (state.names.at ("yw:fx:Maximus") == "2_Max: bus?"); // the name itself, not the file's
    r = edit ([] (yw::plugindb::State& s) { s.names.erase ("yw:fx:Maximus"); });
    CHECK (r.renamed == 1 && t.has (t.fx ("Mix/Bus/Maximus.ywplugin")) && state.names.empty());

    // Rename folder: its entries follow, the old folder goes
    r = edit ([] (yw::plugindb::State& s) { CHECK (yw::plugincatalog::renameFolder (s.folders, false, "Mix", "Mixing")); });
    CHECK (r.moved == 3 && t.has (t.fx ("Mixing/Bus/Maximus.ywplugin")) && ! t.has (t.fx ("Mix")));
    CHECK (state.folders.effects == (std::set<std::string> { "Mixing", "Mixing/Bus" }));

    // a folder made in the file manager since the last refresh is not touched by a change made in YW
    fs::create_directories (t.fx ("Made outside"));
    r = edit ([] (yw::plugindb::State& s) { yw::plugincatalog::addFolder (s.folders, false, "Made in YW"); });
    CHECK (r.foldersMade == 1 && fs::is_directory (t.fx ("Made outside")) && fs::is_directory (t.fx ("Made in YW")));
    CHECK (state.folders.effects.count ("Made outside") && state.folders.effects.count ("Made in YW"));

    // No folder: a built-in one goes back to its default folder, a scanned one loses its entry
    r = edit ([] (yw::plugindb::State& s) {
        yw::plugincatalog::moveToFolder (s.folders, false, "yw:fx:Maximus", "");
        yw::plugincatalog::moveToFolder (s.folders, false, "VST-ProQ-99", "");
    });
    CHECK (r.moved == 1 && r.removed == 1 && t.has (t.fx ("Dynamics/Maximus.ywplugin")) && state.folders.hidden.empty());
    CHECK (entryFiles (t.fx ("Mixing/Bus")) == 1);

    // Delete folder (its plugins go back): the folder and its subfolder are removed, another file in it keeps it
    {
        std::ofstream note (t.fx ("Made outside/notes.txt"));
        note << "mine";
    }
    r = edit ([&] (yw::plugindb::State& s) {
        std::vector<std::string> keys;
        for (const auto& p : all) keys.push_back (p.key);
        yw::plugincatalog::removeFolder (s.folders, false, "Mixing", keys);
        yw::plugincatalog::removeFolder (s.folders, false, "Made outside", keys);
    });
    CHECK (r.foldersRemoved == 2 && ! t.has (t.fx ("Mixing")) && t.has (t.fx ("Made outside/notes.txt")));
    CHECK (! state.folders.of.count ("VST3-ProQ-1234") && ! state.folders.effects.count ("Mixing"));

    // a plugin taken out in the file manager comes back when it is filed in YW
    fs::remove (t.fx ("YW Studio/Facet EQ.ywplugin"));
    yw::plugindb::refresh (t.root, all, state);
    CHECK (state.folders.hidden.count ("yw:fx:Facet EQ"));
    edit ([] (yw::plugindb::State& s) { yw::plugincatalog::moveToFolder (s.folders, false, "yw:fx:Facet EQ", "Back"); });
    CHECK (t.has (t.fx ("Back/Facet EQ.ywplugin")) && state.folders.hidden.empty() && state.folders.of.at ("yw:fx:Facet EQ") == "Back");

    // a folder name a file system cannot hold is stored under a safe one, and the model follows the tree
    edit ([] (yw::plugindb::State& s) { yw::plugincatalog::moveToFolder (s.folders, true, "CLAP-synth-1", "What?"); });
    CHECK (t.has (t.gen ("What_/Vital.ywplugin")) && state.folders.of.at ("CLAP-synth-1") == "What_");
}

TEST (plugindb190ba_odd_trees)
{
    using namespace plugindb190ba;
    TempRoot t;
    yw::plugindb::State state;
    yw::plugindb::refresh (t.root, plugins(), state);
    // the same plugin in two folders: the first path counts, the other file is left alone
    fs::create_directories (t.fx ("A"));
    fs::create_directories (t.fx ("B"));
    fs::copy_file (t.fx ("Dynamics/Maximus.ywplugin"), t.fx ("A/Maximus.ywplugin"));
    fs::copy_file (t.fx ("Dynamics/Maximus.ywplugin"), t.fx ("B/Maximus.ywplugin"));
    // an effect's entry dropped under Generators, a file that is not an entry, an entry at the top, a folder too deep
    fs::copy_file (t.fx ("Dynamics/Maximus.ywplugin"), t.gen ("Maximus.ywplugin"));
    {
        std::ofstream junk (t.fx ("A/broken.ywplugin"));
        junk << "not an entry";
    }
    fs::rename (t.fx ("YW Studio/Facet EQ.ywplugin"), t.fx ("Facet EQ.ywplugin"));
    fs::create_directories (t.fx ("1/2/3/4/5/6"));
    fs::copy_file (u8 (t.root) / "Installed/Effects/VST3/Pro-Q 3.ywplugin", t.fx ("1/2/3/4/5/6/Pro-Q 3.ywplugin"));
    const auto r = yw::plugindb::refresh (t.root, plugins(), state);
    CHECK (r.ok && r.created == 0);
    CHECK (state.folders.of.at ("yw:fx:Maximus") == "A" && t.has (t.fx ("B/Maximus.ywplugin")) && t.has (t.fx ("Dynamics/Maximus.ywplugin")));
    CHECK (! state.folders.of.count ("yw:fx:Facet EQ") && state.folders.hidden.empty()); // at the top: its usual place in the menus
    CHECK (state.folders.of.at ("VST3-ProQ-1234") == "1/2/3/4" && state.folders.effects.count ("1/2/3/4") && ! state.folders.effects.count ("1/2/3/4/5"));
    CHECK (state.folders.generators.empty());
    // a change in YW leaves all of that where it is
    const auto a = yw::plugindb::apply (t.root, plugins(), state);
    CHECK (a.ok && a.moved == 0 && a.removed == 0 && a.created == 0 && a.renamed == 0 && a.foldersRemoved == 0);
    // no folder to write to: nothing happens, the model stays
    auto keep = state;
    CHECK (! yw::plugindb::refresh ("", plugins(), keep).ok && keep == state);
    {
        std::ofstream file (t.dir / "a file");
        file << "x";
    }
    CHECK (! yw::plugindb::refresh (str (t.dir / "a file" / "db"), plugins(), keep).ok && keep == state);
    // an index lost with the tree still there: the tree is read, not made again from the settings
    fs::remove (u8 (t.root) / "database.index");
    yw::plugindb::State stale;
    yw::plugincatalog::moveToFolder (stale.folders, false, "yw:fx:Maximus", "Old place");
    const auto lost = yw::plugindb::refresh (t.root, plugins(), stale);
    CHECK (lost.ok && ! lost.migrated && stale.folders.of.at ("yw:fx:Maximus") == "A" && ! t.has (t.fx ("Old place")));
}
