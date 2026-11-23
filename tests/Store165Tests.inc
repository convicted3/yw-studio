// Round 165n (store165): the Microsoft Store (MSIX) build's folder rules (core/StorePackage.h).

#include "core/StorePackage.h"

TEST (store165_packaged_settings_folder)
{
    namespace st = yw::store;
    CHECK (st::packagedSettingsFolder ("C:\\Users\\ana\\AppData\\Local", "YW.YW_abc123")
           == "C:\\Users\\ana\\AppData\\Local\\Packages\\YW.YW_abc123\\LocalCache\\Roaming\\YW");
    CHECK (st::packagedSettingsFolder ("C:\\Users\\ana\\AppData\\Local\\", "F_x")
           == "C:\\Users\\ana\\AppData\\Local\\Packages\\F_x\\LocalCache\\Roaming\\YW");
    CHECK (st::packagedSettingsFolder ("", "F_x").empty());
    CHECK (st::packagedSettingsFolder ("C:\\L", "").empty());
    CHECK (st::appsFolderTarget ("YW.YW_abc123") == "shell:AppsFolder\\YW.YW_abc123!YW");
}

TEST (store165_migration_keeps_preferences_not_caches)
{
    namespace st = yw::store;
    // preferences, plugin list, favourites, presets, themes, layouts, the user's plugin folder come along
    for (const auto* name : { "plugins.xml", "plugin-favourites.txt", "ui-options.txt", "keys.json", "audio-device.xml",
                              "recentes.txt", "crash-reports.txt", "setup-done.txt", "browser-folders.txt", "workspace.txt" })
        CHECK (st::migratesFromUnpackaged (name, false));
    for (const auto* name : { "Presets", "Themes", "layouts", "Plugins" })
        CHECK (st::migratesFromUnpackaged (name, true));
    // caches, autosaves, crash reports, session markers and logs stay
    for (const auto* name : { "wavecache", "autosave", "flp-embedded", "crash-reports", "sessions" })
        CHECK (! st::migratesFromUnpackaged (name, true));
    for (const auto* name : { "session-running.txt", "ui.log", "audio-glitches.log", "startup.log", "x.tmp", "migrated-from-appdata.txt", "package-probe.txt", "", ".", ".." })
        CHECK (! st::migratesFromUnpackaged (name, false));
}
