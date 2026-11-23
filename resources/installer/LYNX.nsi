; YW Studio per-user Windows installer (NSIS 3). Built by scripts/make-installer.sh; no admin rights needed.
; Installs to %LOCALAPPDATA%\Programs\YW Studio, adds Start-menu (and optional desktop) shortcuts, registers .lynx
; projects and offers YW Studio in Explorer's "Open with" for .flp without taking over FL Studio's default.
; The uninstaller removes only what this installer added (settings in %APPDATA%\YW Studio stay).
;
; Defines passed by the script / release workflow: VERSION (e.g. 0.1.3), STAGE (folder with YW Studio.exe, LICENSE,
; THIRD_PARTY.md, optional PRIVACY.md and Demo.lynx), OUTFILE, ICON.

Unicode true
SetCompressor /SOLID lzma
RequestExecutionLevel user
ManifestDPIAware true

!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "Sections.nsh"
!include "TextFunc.nsh"

!define APP "YW Studio" ; the product name shown (v0.1.20 rename); folders, keys and YW Studio.exe keep "YW Studio"
!define PROGID "YW Studio.Project"
!define FLP_PROGID "YW Studio.FLProject"
!define UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\YW Studio"

Name "${APP} ${VERSION}"
OutFile "${OUTFILE}"
InstallDir "$LOCALAPPDATA\Programs\YW Studio"
InstallDirRegKey HKCU "Software\YW Studio" "InstallDir"
BrandingText "YW Studio ${VERSION}"
VIProductVersion "${VERSION}.0"
VIAddVersionKey "ProductName" "YW Studio"
VIAddVersionKey "FileDescription" "YW Studio installer"
VIAddVersionKey "ProductVersion" "${VERSION}"
VIAddVersionKey "FileVersion" "${VERSION}"
VIAddVersionKey "LegalCopyright" "AGPL-3.0"

!define MUI_ICON "${ICON}"
!define MUI_UNICON "${ICON}"
!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_RUN "$INSTDIR\YW Studio.exe"
!define MUI_FINISHPAGE_RUN_TEXT "$(RunLynx)"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${STAGE}\LICENSE"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "PortugueseBR"

LangString RunLynx ${LANG_ENGLISH} "Start YW Studio"
LangString RunLynx ${LANG_PORTUGUESEBR} "Abrir o YW Studio"
LangString SecApp ${LANG_ENGLISH} "YW Studio (required)"
LangString SecApp ${LANG_PORTUGUESEBR} "YW Studio (obrigatório)"
LangString SecDesktop ${LANG_ENGLISH} "Desktop shortcut"
LangString SecDesktop ${LANG_PORTUGUESEBR} "Atalho na área de trabalho"
LangString SecFlp ${LANG_ENGLISH} "Offer YW Studio in $\"Open with$\" for FL Studio projects (.flp)"
LangString SecFlp ${LANG_PORTUGUESEBR} "Oferecer o YW Studio em $\"Abrir com$\" para projetos do FL Studio (.flp)"
LangString FlpType ${LANG_ENGLISH} "FL Studio project (YW Studio)"
LangString FlpType ${LANG_PORTUGUESEBR} "Projeto do FL Studio (YW Studio)"
LangString StillRunning ${LANG_ENGLISH} "YW Studio is running. Close it and click Retry."
LangString StillRunning ${LANG_PORTUGUESEBR} "O YW Studio está aberto. Feche-o e clique em Repetir."

Function .onInit
    ; the installer's language follows Windows
    System::Call 'kernel32::GetUserDefaultUILanguage() i .r0'
    ${If} $0 == 1046
        StrCpy $LANGUAGE ${LANG_PORTUGUESEBR}
    ${EndIf}
    Call SilentKeepsChoices
FunctionEnd

!macro CheckNotRunning
    ; overwriting a running exe fails: ask to close it first
    retry:
    ClearErrors
    ${If} ${FileExists} "$INSTDIR\YW Studio.exe"
        FileOpen $0 "$INSTDIR\YW Studio.exe" a
        ${If} ${Errors}
            MessageBox MB_RETRYCANCEL|MB_ICONEXCLAMATION "$(StillRunning)" IDRETRY retry
            Abort
        ${EndIf}
        FileClose $0
    ${EndIf}
!macroend

!include "InstalledFiles.nsh" ; round 190nr: installed-files.txt, removing a newer version's leftovers

Section "!$(SecApp)" SecMain
    SectionIn RO
    SetShellVarContext current
    !insertmacro CheckNotRunning
    SetOutPath "$INSTDIR"
    Call RemoveNewerLeftovers
    File "${STAGE}\YW Studio.exe"
    File /nonfatal "${STAGE}\onnxruntime.dll" ; Extract stems from sample
    File "${STAGE}\LICENSE"
    File "${STAGE}\THIRD_PARTY.md"
    File /nonfatal "${STAGE}\PRIVACY.md"
    SetOutPath "$INSTDIR\licenses" ; third-party licence texts (scripts/collect-licenses.sh, round 190nk)
    File /nonfatal /r "${STAGE}\licenses\*"
    SetOutPath "$INSTDIR"
    File /nonfatal "${STAGE}\Demo.lynx"
    File /nonfatal "${STAGE}\YW-bridge32.exe" ; 32-bit plugins run in it (round 176a)
    ; YW ASIO (REA round 97): YW Studio loads it directly; the .bat files register it for other programs (administrator)
    File /nonfatal "${STAGE}\lynx-asio.dll"
    File /nonfatal "${STAGE}\lynx-asio32.dll" ; for 32-bit programs (REA round 106)
    File /nonfatal "${STAGE}\Register YW ASIO.bat"
    File /nonfatal "${STAGE}\Unregister YW ASIO.bat"
    WriteUninstaller "$INSTDIR\Uninstall.exe"
    Call WriteInstalledFiles

    Delete "$SMPROGRAMS\YW Studio.lnk" ; the shortcut before the v0.1.20 rename
    CreateShortCut "$SMPROGRAMS\YW Studio.lnk" "$INSTDIR\YW Studio.exe" "" "$INSTDIR\YW Studio.exe" 0

    ; .lynx projects (YW Studio itself keeps this up to date at every start, too)
    WriteRegStr HKCU "Software\Classes\.lynx" "" "${PROGID}"
    WriteRegStr HKCU "Software\Classes\.lynx" "Content Type" "application/x-lynx-project"
    WriteRegStr HKCU "Software\Classes\${PROGID}" "" "YW Studio Project"
    WriteRegStr HKCU "Software\Classes\${PROGID}\DefaultIcon" "" '"$INSTDIR\YW Studio.exe",-2'
    WriteRegStr HKCU "Software\Classes\${PROGID}\shell\open\command" "" '"$INSTDIR\YW Studio.exe" "%1"'
    ; "Open with" list entry for the app
    WriteRegStr HKCU "Software\Classes\Applications\YW Studio.exe" "FriendlyAppName" "YW Studio"
    WriteRegStr HKCU "Software\Classes\Applications\YW Studio.exe\shell\open\command" "" '"$INSTDIR\YW Studio.exe" "%1"'
    WriteRegStr HKCU "Software\Classes\Applications\YW Studio.exe\SupportedTypes" ".lynx" ""

    WriteRegStr HKCU "Software\YW Studio" "InstallDir" "$INSTDIR"
    WriteRegStr HKCU "${UNINST_KEY}" "DisplayName" "YW Studio"
    WriteRegStr HKCU "${UNINST_KEY}" "DisplayVersion" "${VERSION}"
    WriteRegStr HKCU "${UNINST_KEY}" "Publisher" "YW Studio"
    WriteRegStr HKCU "${UNINST_KEY}" "URLInfoAbout" "https://github.com/convicted3/yw-studio"
    WriteRegStr HKCU "${UNINST_KEY}" "DisplayIcon" '"$INSTDIR\YW Studio.exe",0'
    WriteRegStr HKCU "${UNINST_KEY}" "InstallLocation" "$INSTDIR"
    WriteRegStr HKCU "${UNINST_KEY}" "UninstallString" '"$INSTDIR\Uninstall.exe"'
    WriteRegStr HKCU "${UNINST_KEY}" "QuietUninstallString" '"$INSTDIR\Uninstall.exe" /S'
    WriteRegDWORD HKCU "${UNINST_KEY}" "NoModify" 1
    WriteRegDWORD HKCU "${UNINST_KEY}" "NoRepair" 1
    SectionGetSize ${SecMain} $0
    WriteRegDWORD HKCU "${UNINST_KEY}" "EstimatedSize" $0
SectionEnd

Section "$(SecDesktop)" SecDesktop
    SetShellVarContext current
    Delete "$DESKTOP\YW Studio.lnk"
    CreateShortCut "$DESKTOP\YW Studio.lnk" "$INSTDIR\YW Studio.exe" "" "$INSTDIR\YW Studio.exe" 0
SectionEnd

Section "$(SecFlp)" SecFlp
    ; adds YW Studio to .flp's "Open with" list only: FL Studio stays the default program for .flp
    WriteRegStr HKCU "Software\Classes\${FLP_PROGID}" "" "$(FlpType)"
    WriteRegStr HKCU "Software\Classes\${FLP_PROGID}\DefaultIcon" "" '"$INSTDIR\YW Studio.exe",0'
    WriteRegStr HKCU "Software\Classes\${FLP_PROGID}\shell\open\command" "" '"$INSTDIR\YW Studio.exe" "%1"'
    WriteRegStr HKCU "Software\Classes\.flp\OpenWithProgids" "${FLP_PROGID}" ""
    WriteRegStr HKCU "Software\Classes\Applications\YW Studio.exe\SupportedTypes" ".flp" ""
SectionEnd

Section "-notify"
    System::Call 'shell32::SHChangeNotify(i 0x08000000, i 0, p 0, p 0)'
SectionEnd

Section "Uninstall"
    SetShellVarContext current
    Delete "$INSTDIR\YW Studio.exe"
    Delete "$INSTDIR\YW Studio.pdb"
    Delete "$INSTDIR\LICENSE"
    Delete "$INSTDIR\THIRD_PARTY.md"
    Delete "$INSTDIR\PRIVACY.md"
    RMDir /r "$INSTDIR\licenses"
    Delete "$INSTDIR\Demo.lynx"
    ; YW ASIO registered for other programs: unregister it first (Windows asks for administrator rights).
    ; The 64-bit registration lives in the 64-bit registry view (this uninstaller is a 32-bit program).
    SetRegView 64
    ReadRegStr $0 HKLM "SOFTWARE\ASIO\YW ASIO" "CLSID"
    SetRegView 32
    ${If} $0 != ""
    ${AndIf} ${FileExists} "$INSTDIR\lynx-asio.dll"
        ExecShellWait "runas" "$WINDIR\Sysnative\regsvr32.exe" '/u /s "$INSTDIR\lynx-asio.dll"'
    ${EndIf}
    ; the 32-bit registration lives in the 32-bit registry view
    ReadRegStr $0 HKLM "SOFTWARE\ASIO\YW ASIO" "CLSID"
    ${If} $0 != ""
    ${AndIf} ${FileExists} "$INSTDIR\lynx-asio32.dll"
        ExecShellWait "runas" "$WINDIR\SysWOW64\regsvr32.exe" '/u /s "$INSTDIR\lynx-asio32.dll"'
    ${EndIf}
    Delete "$INSTDIR\onnxruntime.dll"
    Delete "$INSTDIR\YW-bridge32.exe"
    Delete "$INSTDIR\lynx-asio.dll"
    Delete "$INSTDIR\lynx-asio32.dll"
    Delete "$INSTDIR\Register YW ASIO.bat"
    Delete "$INSTDIR\Unregister YW ASIO.bat"
    Delete "$INSTDIR\installed-files.txt"
    Delete "$INSTDIR\Uninstall.exe"
    RMDir "$INSTDIR"
    Delete "$SMPROGRAMS\YW Studio.lnk"
    Delete "$DESKTOP\YW Studio.lnk"
    Delete "$SMPROGRAMS\YW Studio.lnk"
    Delete "$DESKTOP\YW Studio.lnk"

    ; only our own keys and values; .lynx is removed only while it still points at YW Studio
    ReadRegStr $0 HKCU "Software\Classes\.lynx" ""
    ${If} $0 == "${PROGID}"
        DeleteRegKey HKCU "Software\Classes\.lynx"
    ${EndIf}
    DeleteRegKey HKCU "Software\Classes\${PROGID}"
    DeleteRegKey HKCU "Software\Classes\${FLP_PROGID}"
    DeleteRegValue HKCU "Software\Classes\.flp\OpenWithProgids" "${FLP_PROGID}"
    ; (the .flp key itself is FL Studio's: left alone)
    DeleteRegKey HKCU "Software\Classes\Applications\YW Studio.exe"
    DeleteRegKey HKCU "Software\YW Studio"
    DeleteRegKey HKCU "${UNINST_KEY}"
    System::Call 'shell32::SHChangeNotify(i 0x08000000, i 0, p 0, p 0)'
SectionEnd

; A silent install (/S) is YW Studio's in-app update (ui/Updater.h, round 166b): it keeps the choices of the install it
; updates instead of the defaults, so an update never adds a desktop shortcut or the .flp "Open with" entry the user
; left out. (Defined after the sections: it names their indices.)
Function SilentKeepsChoices
    ${If} ${Silent}
        SetShellVarContext current
        ${IfNot} ${FileExists} "$DESKTOP\YW Studio.lnk"
            !insertmacro UnselectSection ${SecDesktop}
        ${EndIf}
        ReadRegStr $0 HKCU "Software\Classes\${FLP_PROGID}" ""
        ${If} $0 == ""
            !insertmacro UnselectSection ${SecFlp}
        ${EndIf}
    ${EndIf}
FunctionEnd
