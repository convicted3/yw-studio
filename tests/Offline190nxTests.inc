// --- Round 190nx: offline mode (core/Offline.h): YW_OFFLINE=1 or --offline. The end-to-end proof (zero connect() calls under
// `unshare -rn` + strace) is tests/offline-network-test.sh with `YW --offlinetest`.

#include "core/Offline.h"

TEST (offline190nx_the_environment_variable_and_the_flag_turn_the_mode_on)
{
    namespace off = yw::offline;
    const char* before = std::getenv ("YW_OFFLINE");
    const std::string saved = before != nullptr ? before : "";
    auto setEnv = [] (const char* v) {
       #if defined(_WIN32)
        _putenv_s ("YW_OFFLINE", v);
       #else
        if (v[0] == '\0')
            unsetenv ("YW_OFFLINE");
        else
            setenv ("YW_OFFLINE", v, 1);
       #endif
    };
    off::set (false);
    setEnv ("");
    CHECK (! off::enabled());
    for (const char* on : { "1", "true", "yes", "on", "anything" })
    {
        setEnv (on);
        CHECK (off::enabled());
    }
    for (const char* no : { "0", "false", "no", "off" })
    {
        setEnv (no);
        CHECK (! off::enabled());
    }
    setEnv ("");
    off::set (true); // --offline
    CHECK (off::enabled());
    off::set (false);
    CHECK (! off::enabled());
    setEnv (saved.c_str());
}
