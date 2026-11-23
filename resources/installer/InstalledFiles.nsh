; Included by YW Studio.nsi (and by the test installer of round 190nr). Needs LogicLib.nsh and TextFunc.nsh.
;; Round 190nr: what this installer owns in $INSTDIR (top-level names). Every name a File command in YW Studio.nsi writes must be
;; here, and core/UpdateCheck.cpp installEntries() needs every name that is new in this version. The list is written
;; to installed-files.txt; an install over a NEWER version reads that version's list first and removes the names this
;; version doesn't own, so going back leaves nothing of the newer version behind (0.1.29's installer left 0.1.30's
;; licenses\). Installers of 0.1.30 and older don't do this: for them the newer version's --apply-update helper cleans
;; up (src/ui/Updater.cpp removeNewerLeftovers). A 0.1.30 or older setup run by hand over a newer version can't be cleaned
;; up from the newer side (its script is fixed and the newer version doesn't run); what it leaves is harmless and goes
;; with the next 0.1.31+ install or uninstall.
!macro ForEachOwned M
    !insertmacro ${M} "YW Studio.exe"
    !insertmacro ${M} "YW Studio.pdb"
    !insertmacro ${M} "onnxruntime.dll"
    !insertmacro ${M} "LICENSE"
    !insertmacro ${M} "THIRD_PARTY.md"
    !insertmacro ${M} "PRIVACY.md"
    !insertmacro ${M} "licenses"
    !insertmacro ${M} "Demo.yw"
    !insertmacro ${M} "YW-bridge32.exe"
    !insertmacro ${M} "yw-asio.dll"
    !insertmacro ${M} "yw-asio32.dll"
    !insertmacro ${M} "Register YW ASIO.bat"
    !insertmacro ${M} "Unregister YW ASIO.bat"
    !insertmacro ${M} "Uninstall.exe"
    !insertmacro ${M} "installed-files.txt"
!macroend
!macro IsOwnedName NAME
    ${If} $R1 == "${NAME}"
        StrCpy $R2 1
    ${EndIf}
!macroend
!macro WriteOwnedName NAME
    FileWrite $R0 "${NAME}$\r$\n"
!macroend

; $R1 = a name from installed-files.txt -> $R2 = 1 when it is a plain top-level name this installer does not own
Function IsStrayName
    StrCpy $R2 0
    !insertmacro ForEachOwned IsOwnedName
    ${If} $R2 == 1
        StrCpy $R2 0
        Return
    ${EndIf}
    ${If} $R1 == ""
    ${OrIf} $R1 == "."
    ${OrIf} $R1 == ".."
        Return
    ${EndIf}
    StrLen $R3 $R1
    StrCpy $R4 0
    ${DoWhile} $R4 < $R3
        StrCpy $R5 $R1 1 $R4
        ${If} $R5 == "\"
        ${OrIf} $R5 == "/"
        ${OrIf} $R5 == ":"
            Return
        ${EndIf}
        IntOp $R4 $R4 + 1
    ${Loop}
    StrCpy $R2 1
FunctionEnd

Function RemoveNewerLeftovers
    ClearErrors
    FileOpen $R0 "$INSTDIR\installed-files.txt" r
    ${If} ${Errors}
        Return
    ${EndIf}
    ${Do}
        ClearErrors
        FileRead $R0 $R1
        ${If} ${Errors}
            ${ExitDo}
        ${EndIf}
        ${TrimNewLines} $R1 $R1
        Call IsStrayName
        ${If} $R2 == 1
            ${If} ${FileExists} "$INSTDIR\$R1\*.*"
                RMDir /r "$INSTDIR\$R1"
            ${Else}
                Delete "$INSTDIR\$R1"
            ${EndIf}
        ${EndIf}
    ${Loop}
    FileClose $R0
FunctionEnd

Function WriteInstalledFiles
    FileOpen $R0 "$INSTDIR\installed-files.txt" w
    !insertmacro ForEachOwned WriteOwnedName
    FileClose $R0
FunctionEnd
