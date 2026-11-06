#include "MenuModel.h"

using namespace cmd;

namespace
{
    struct Builder
    {
        MenuModel& model;
        juce::PopupMenu& menu;

        void item (int id, const juce::String& text, const juce::String& shortcut = {})
        {
            juce::PopupMenu::Item i (text);
            i.itemID = id;
            i.shortcutKeyDescription = shortcut;
            i.isEnabled = model.isEnabled ? model.isEnabled (id) : false;
            i.isTicked  = model.isTicked  ? model.isTicked (id)  : false;
            menu.addItem (i);
        }

        void header (const juce::String& text) { menu.addSectionHeader (text); }
        void sep() { menu.addSeparator(); }
    };
}

juce::StringArray MenuModel::getMenuBarNames()
{
    return { "FILE", "EDIT", "ADD", "PATTERNS", "VIEW", "OPTIONS", "TOOLS", "HELP" };
}

juce::PopupMenu MenuModel::getMenuForIndex (int index, const juce::String&)
{
    juce::PopupMenu m;
    Builder b { *this, m };

    switch (index)
    {
        case 0: // FILE
        {
            b.item (fileNew, "New");
            juce::PopupMenu tpl; Builder t { *this, tpl };
            t.item (fileNewFromTemplate, "Empty");
            m.addSubMenu ("New from template", tpl);
            b.sep();
            b.item (fileOpen, "Open...", "Ctrl+O");
            b.item (fileSave, "Save", "Ctrl+S");
            b.item (fileSaveAs, "Save as...", "Shift+Ctrl+S");
            b.item (fileSaveNewVersion, "Save new version", "Ctrl+N");
            b.item (fileSaveAsTemplate, "Save as template...");
            b.sep();
            juce::PopupMenu imp; Builder i { *this, imp };
            i.item (fileImportMidi, "MIDI file...");
            i.item (fileImportFlp, "FL Studio project (.flp)...");
            m.addSubMenu ("Import", imp);
            juce::PopupMenu exp; Builder e { *this, exp };
            e.item (fileExportWav, "Wave file...", "Ctrl+R");
            e.item (fileExportMp3, "MP3 file...", "Shift+Ctrl+R");
            e.item (fileExportOgg, "OGG file...");
            e.item (fileExportFlac, "FLAC file...");
            e.item (fileExportMidi, "MIDI file...", "Shift+Ctrl+M");
            e.sep();
            e.item (fileExportStems, "All playlist tracks...");
            e.item (fileExportZipped, "Zipped loop package...");
            m.addSubMenu ("Export", exp);
            b.sep();
            m.addSubMenu ("Recent projects", juce::PopupMenu(), false);
            b.item (fileRevertBackup, "Revert to last backup");
            b.sep();
            b.item (fileExit, "Exit", "Alt+F4");
            break;
        }

        case 1: // EDIT
            b.item (editUndo, "Undo", "Ctrl+Z");
            b.item (editRedo, "Redo", "Ctrl+Alt+Z");
            b.item (editUndoHistory, "Undo history");
            b.sep();
            b.item (editCut, "Cut", "Ctrl+X");
            b.item (editCopy, "Copy", "Ctrl+C");
            b.item (editPaste, "Paste", "Ctrl+V");
            b.item (editDelete, "Delete", "Del");
            break;

        case 2: // ADD
        {
            juce::PopupMenu ch; Builder c { *this, ch };
            c.item (addChannel, "More plugins...");
            m.addSubMenu ("Channel", ch);
            juce::PopupMenu fx; Builder f { *this, fx };
            f.item (addEffect, "More plugins...");
            m.addSubMenu ("Effect", fx);
            b.sep();
            b.item (addPluginPicker, "View plugin picker", "F8");
            b.item (addBrowseLibrary, "Browse plugin library", "Shift+F8");
            b.item (addBrowseInstalled, "Browse all installed plugins");
            b.item (addBrowsePresets, "Browse presets");
            b.sep();
            b.item (addRefreshPlugins, "Refresh plugins list (fast scan)");
            b.item (addManagePlugins, "Manage plugins...");
            b.sep();
            b.item (addAutomationClip, "Automation clip for last tweaked parameter");
            b.item (addPattern, "Pattern");
            break;
        }

        case 3: // PATTERNS
            b.item (patFindFirstEmpty, "Find first empty", "Shift+F4");
            b.item (patFindNextEmpty, "Find next empty", "F4");
            b.item (patFindNextEmptyNoName, "Find next empty (no naming)", "Ctrl+4");
            b.item (patSelectInPlaylist, "Select in playlist");
            b.sep();
            b.item (patRename, "Rename and color...", "F2");
            b.item (patRandomColor, "Random color");
            b.item (patOpenInBrowser, "Open in project browser", "Shift+Ctrl+P");
            b.sep();
            b.item (patGroup, "Group selected");
            b.item (patUngroup, "Ungroup selected");
            b.sep();
            b.item (patTimeSig, "Set time signature...");
            b.item (patTranspose, "Transpose...");
            b.sep();
            b.item (patInsert, "Insert one", "Shift+Ctrl+Ins");
            b.item (patClone, "Clone", "Alt+C");
            b.item (patDelete, "Delete...", "Del");
            b.item (patMoveUp, "Move up", "Shift+Ctrl+Up");
            b.item (patMoveDown, "Move down", "Shift+Ctrl+Down");
            b.sep();
            b.item (patSplitByChannel, "Split by channel");
            b.item (patRenderAudio, "Render as audio clip");
            break;

        case 4: // VIEW
        {
            b.header ("Windows");
            b.item (viewPlaylist, "Playlist", "F5");
            b.item (viewChannelRack, "Channel rack", "F6");
            b.item (viewPianoRoll, "Piano roll", "F7");
            b.item (viewPluginPicker, "Plugin picker", "F8");
            b.item (viewBrowser, "Browser", "Alt+F8");
            b.item (viewProjectPicker, "Project picker", "Ctrl+F8");
            b.item (viewMixer, "Mixer", "F9");
            b.item (viewTempoTapper, "Tempo tapper");
            juce::PopupMenu tb; Builder t { *this, tb };
            t.item (viewToolbarHide, "Hide", "Ctrl+F11");
            m.addSubMenu ("Toolbars", tb);
            b.item (viewPerfMonitor, "Plugin performance monitor");
            b.header ("Layout");
            b.item (viewCloseAll, "Close all windows", "F12");
            b.item (viewCloseAllPlugins, "Close all plugin windows", "Alt+F12");
            b.item (viewCloseUnfocused, "Close all unfocused windows", "Ctrl+F12");
            b.item (viewAlignEditors, "Align all channel editors", "Shift+F12");
            juce::PopupMenu ar; Builder a { *this, ar };
            a.item (viewArrangeRealign, "Realign", "Shift+Ctrl+H");
            a.item (viewArrangeDefault, "Default");
            m.addSubMenu ("Arrange windows", ar);
            b.header ("Browser");
            b.item (viewBrowserHistory, "Undo history");
            b.item (viewBrowserPatterns, "Patterns");
            b.item (viewBrowserGenerators, "Generators in use");
            b.item (viewBrowserEffects, "Effects in use");
            b.item (viewBrowserLibrary, "Plugin library", "Shift+F8");
            break;
        }

        case 5: // OPTIONS
            b.header ("System");
            b.item (optMidi, "MIDI settings", "F10");
            b.item (optAudio, "Audio settings");
            b.item (optGeneral, "General settings");
            b.item (optFile, "File settings");
            b.item (optTheme, "Theme settings");
            b.item (optManagePlugins, "Manage plugins");
            b.header ("Project");
            b.item (optProjectInfo, "Project info", "F11");
            b.item (optProjectGeneral, "Project general settings");
            b.header ("Info");
            b.item (optDebugLog, "Debugging log");
            b.header ("MIDI");
            b.item (optMidiRemote, "Enable MIDI remote control");
            b.item (optMidiOut, "Enable MIDI output");
            b.item (optMidiSync, "Enable MIDI master sync");
            b.header ("Switches");
            b.item (optTypingKeyboard, "Typing keyboard to piano keyboard", "Ctrl+T");
            b.item (optMetronome, "Metronome", "Ctrl+M");
            b.item (optPrecount, "Recording precount", "Ctrl+P");
            b.item (optStartOnInput, "Start on input", "Ctrl+I");
            b.item (optBlendNotes, "Blend recorded notes", "Ctrl+B");
            b.item (optStepEdit, "Step edit", "Ctrl+E");
            b.item (optLoopRecord, "Loop record");
            b.item (optAutoScroll, "Auto scrolling");
            b.item (optEnableGroups, "Enable groups");
            b.item (optMultilink, "Multilink to controllers", "Ctrl+J");
            break;

        case 6: // TOOLS
        {
            b.item (toolFind, "Browser find", "Ctrl+F");
            b.item (toolOneClickRecord, "One-click audio recording");
            b.item (toolIdeaGenerator, "Idea generator");
            b.item (toolChordProgression, "Chord progression");
            juce::PopupMenu mac; Builder mc { *this, mac };
            mc.item (toolPanicStop, "Panic: stop sound", "Ctrl+H");
            mc.item (toolPanicCancelRec, "Panic: cancel recording");
            mc.sep();
            mc.item (toolSelectUnused, "Select unused channels");
            mc.item (toolPurgeUnusedClips, "Purge unused audio clips");
            m.addSubMenu ("Macros", mac);
            juce::PopupMenu clip; Builder cl { *this, clip };
            cl.item (toolSaveClipboardToBrowser, "Save audio/MIDI clipboard to browser");
            m.addSubMenu ("Clipboard", clip);
            juce::PopupMenu score; Builder sc { *this, score };
            sc.item (toolScoreLogDump, "Dump score log to selected pattern");
            sc.item (toolScoreLogClear, "Clear log");
            m.addSubMenu ("Score logger", score);
            b.item (toolAudioLogger, "Audio logger");
            juce::PopupMenu lt; Builder l { *this, lt };
            l.item (toolLastTweakedEvents, "Edit events");
            l.item (toolLastTweakedAutomation, "Create automation clip");
            l.item (toolLastTweakedLink, "Link to controller...");
            m.addSubMenu ("Last tweaked", lt);
            b.sep();
            b.item (toolExternal, "External tools...");
            b.item (toolPluginCatalog, "YW plugin catalog...");
            break;
        }

        case 7: // HELP
            b.item (helpIndex, "Help index", "F1");
            b.item (helpTutorials, "Tutorials");
            b.item (helpWhatsNew, "What's new");
            b.sep();
            b.item (helpUpdates, "Check for updates");
            b.item (helpWebsite, "YW Studio website (yolo.su)");
            b.item (helpGithub, "Source code and issues (GitHub)");
            b.sep();
            b.item (helpAbout, "About YW Studio");
            break;

        default: break;
    }

    return m;
}

void MenuModel::menuItemSelected (int id, int)
{
    if (onCommand) onCommand (id);
}
