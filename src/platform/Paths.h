#pragma once

// Per-OS folders (docs/PLATFORMS.md). Everything platform-specific about *where* YW keeps or looks for files
// lives here, so the rest of the code asks for "the settings folder" instead of spelling out %APPDATA%.

#include <juce_core/juce_core.h>

namespace yw::platform
{
// YW's settings folder (created on first use):
//   Windows  %APPDATA%\YW
//   macOS    ~/Library/Application Support/YW
//   Linux    $XDG_CONFIG_HOME/YW (default ~/.config/YW)
juce::File settingsFolder();

// YW_PROFILE_DIR (core/TestProfile.h): the folder that replaces every user-data location, or an empty File.
juce::File profileOverride();
// YW_TEST_MODE: no prompts (they are logged to test-mode.log in the settings folder instead).
bool testMode();
void testModeLog (const juce::String& what);

// The user's Documents folder for what YW writes there (Documents/YW/...): <profile>/Documents under a test profile.
juce::File documentsFolder();

// A file or sub-folder inside the settings folder, e.g. settingsFile ("autosave").
juce::File settingsFile (const juce::String& relativePath);

// Rebuildable caches (waveform peaks):
//   Windows  %APPDATA%\YW\<name>   (unchanged from earlier versions)
//   macOS    ~/Library/Caches/YW/<name>
//   Linux    $XDG_CACHE_HOME/YW/<name> (default ~/.cache/YW/<name>)
juce::File cacheFolder (const juce::String& name);

// The standard plugin folders of this OS, in scan order (they need not exist yet: plugins installed later land
// there): VST3, VST2, CLAP (+ AU components on macOS, LV2 on Linux) and YW's own Plugins folder.
juce::FileSearchPath standardPluginFolders();

// The plugin formats YW scans for on this OS, for the UI ("VST, VST3, CLAP, LV2" on Linux).
juce::String pluginFormatNames();

// A file named by a path stored in a project or written by the FL import: on Windows as it is; on macOS/Linux FL's
// backslashes become '/' and, when the exact spelling does not exist, the letter case is matched against the disk
// (FL Studio's data was written for a case-insensitive file system).
juce::File localFile (const juce::String& path);
juce::File localFile (const std::string& utf8Path);

// Windows: when YW.exe sits in a folder whose path has non-ASCII characters ("Área de Trabalho"), some plugins
// crash opening their editor (Waves 17's WebView builds a path from the host's module name and throws on it). This
// starts YW again through the folder's 8.3 short name (plain ASCII), waits for it and returns true with its exit
// code: the caller then quits. False (nothing done) when the path is ASCII, has no short name, or on other systems.
bool relaunchFromAsciiPath (int& exitCode);

// Test runs (round 190mo): a temp folder of this process alone, "<temp>/<prefix>-<random>", made the temp folder of
// the process (TMPDIR on Linux / macOS, TMP and TEMP on Windows; child processes inherit it), so test runs at the same
// time never share the fixed temp names their items use. Returns the folder (delete it when the run ends), or an
// empty File when it could not be made (the system temp folder stays).
juce::File useOwnTempFolder (const juce::String& prefix);

// Test runs (round 190mo), Linux: the main thread's stack may grow to `bytes` (the soft RLIMIT_STACK, at most the hard
// limit; the kernel checks it whenever the stack grows). The --selftest builds many AudioEngines (1.5 MB each) on the
// main thread and came within 0.6 MB of the usual 8 MB. Does nothing elsewhere or when the limit is already higher.
void allowMainThreadStack (std::size_t bytes);
// When `file` was created (its birth time), in seconds since 1970 UTC; 0 when the file system does not keep one or the
// file is missing (issue #231, round 190ns: Project info › Created of a project saved without a date). Windows: the
// creation time; macOS: st_birthtime; Linux: statx's STATX_BTIME (ext4, Btrfs, XFS, tmpfs keep it; JUCE's
// getCreationTime gives st_ctime there, the last change of the file's inode).
double fileBirthSeconds (const juce::File& file);
} // namespace yw::platform
