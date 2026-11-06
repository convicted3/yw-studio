#pragma once
#include <JuceHeader.h>

// Main menu ids. Items without a handler yet are shown disabled, in their final place.
namespace cmd
{
    enum Id
    {
        // File
        fileNew = 100, fileNewFromTemplate, fileOpen, fileSave, fileSaveAs, fileSaveNewVersion, fileSaveAsTemplate,
        fileImportMidi, fileImportFlp,
        fileExportWav, fileExportMp3, fileExportOgg, fileExportFlac, fileExportMidi, fileExportStems, fileExportZipped,
        fileRevertBackup, fileExit,

        // Edit
        editUndo = 200, editRedo, editUndoHistory, editCut, editCopy, editPaste, editDelete,

        // Add
        addChannel = 300, addEffect, addPluginPicker, addBrowseLibrary, addBrowseInstalled, addBrowsePresets,
        addRefreshPlugins, addManagePlugins, addAutomationClip, addPattern,

        // Patterns
        patFindFirstEmpty = 400, patFindNextEmpty, patFindNextEmptyNoName, patSelectInPlaylist, patRename, patRandomColor,
        patOpenInBrowser, patGroup, patUngroup, patTimeSig, patTranspose, patInsert, patClone, patDelete,
        patMoveUp, patMoveDown, patSplitByChannel, patRenderAudio,

        // View
        viewPlaylist = 500, viewChannelRack, viewPianoRoll, viewPluginPicker, viewBrowser, viewProjectPicker, viewMixer,
        viewTempoTapper, viewToolbarHide, viewPerfMonitor,
        viewCloseAll, viewCloseAllPlugins, viewCloseUnfocused, viewAlignEditors, viewArrangeDefault, viewArrangeRealign,
        viewBrowserHistory, viewBrowserPatterns, viewBrowserGenerators, viewBrowserEffects, viewBrowserLibrary,

        // Options
        optMidi = 600, optAudio, optGeneral, optFile, optTheme, optManagePlugins, optProjectInfo, optProjectGeneral,
        optDebugLog, optMidiRemote, optMidiOut, optMidiSync,
        optTypingKeyboard, optMetronome, optPrecount, optStartOnInput, optBlendNotes, optStepEdit, optLoopRecord,
        optAutoScroll, optEnableGroups, optMultilink,

        // Tools
        toolFind = 700, toolOneClickRecord, toolIdeaGenerator, toolChordProgression,
        toolPanicStop, toolPanicCancelRec, toolSelectUnused, toolPurgeUnusedClips,
        toolSaveClipboardToBrowser, toolScoreLogDump, toolScoreLogClear, toolAudioLogger,
        toolLastTweakedEvents, toolLastTweakedAutomation, toolLastTweakedLink, toolExternal, toolPluginCatalog,

        // Help
        helpIndex = 800, helpTutorials, helpWhatsNew, helpUpdates, helpWebsite, helpGithub, helpAbout
    };
}

class MenuModel : public juce::MenuBarModel
{
public:
    juce::StringArray getMenuBarNames() override;
    juce::PopupMenu getMenuForIndex (int index, const juce::String& name) override;
    void menuItemSelected (int id, int topLevelIndex) override;

    std::function<void (int)> onCommand;
    std::function<bool (int)> isTicked; // for switches / visible windows
    std::function<bool (int)> isEnabled;
};
