#!/usr/bin/env bash
# macOS package of a built YW.app (shown as YW Studio: CFBundleName): ad-hoc signature (YW has no Apple Developer ID, so it is NOT notarized) and a
# compressed .dmg with the app, a link to /Applications and a "read me first" note on how to open it the first time.
#
# usage: scripts/package-macos.sh <YW.app> <label> <output folder> [Demo.yw]
set -euo pipefail
cd "$(dirname "$0")/.."

APP="$1"; LABEL="$2"; OUTDIR="$3"; DEMO="${4:-}"
NAME="YW-$LABEL-macos-universal"
STAGE="$(mktemp -d)/YW"
mkdir -p "$STAGE" "$OUTDIR"

ditto "$APP" "$STAGE/YW.app"
./scripts/fetch-onnxruntime.sh macos "$STAGE/YW.app/Contents/Frameworks"
# ad-hoc signature over the whole bundle: Apple Silicon refuses to run unsigned code, and a consistent bundle
# signature turns "YW is damaged" into the normal "unidentified developer" prompt
codesign --force --deep --sign - "$STAGE/YW.app"
codesign --verify --deep --strict "$STAGE/YW.app"
ln -s /Applications "$STAGE/Applications"
[[ -n "$DEMO" && -f "$DEMO" ]] && cp "$DEMO" "$STAGE/Demo.yw"
cp LICENSE "$STAGE/LICENSE.txt"
# third-party notices and licence texts, as in the Windows and Linux packages (round 190nk; 0.1.29's .dmg had LICENSE only)
cp THIRD_PARTY.md PRIVACY.md "$STAGE/"
./scripts/collect-licenses.sh "$STAGE/licenses"
cat > "$STAGE/Read me first.txt" <<'EOF'
YW Studio for macOS (Apple Silicon and Intel, macOS 11 or newer)

1. Drag YW Studio into the Applications folder.
2. YW Studio is free software from an independent developer without an Apple Developer ID, so it is not notarized by
   Apple. The first time, open it like this:
     - Control-click (or right-click) YW Studio in Applications, choose Open, then Open again.
     - On macOS 15 Sequoia or newer: try to open YW Studio once, then go to System Settings > Privacy & Security and
       click "Open Anyway" next to the message about YW.
   Or, in Terminal:   xattr -dr com.apple.quarantine /Applications/YW.app
   After that it opens normally.
3. macOS asks for microphone access the first time you record.

Source code, checksums and help: https://github.com/convicted3/yw-studio

---

YW Studio para macOS (Apple Silicon e Intel, macOS 11 ou mais novo)

1. Arraste o YW Studio para a pasta Aplicativos.
2. O YW Studio é software livre de um desenvolvedor independente sem Apple Developer ID, por isso não é notarizado pela
   Apple. Na primeira vez, abra assim:
     - Clique com o botão direito (ou Control-clique) no YW Studio em Aplicativos, escolha Abrir e depois Abrir de novo.
     - No macOS 15 Sequoia ou mais novo: tente abrir o YW Studio uma vez, depois vá em Ajustes do Sistema > Privacidade e
       Segurança e clique em "Abrir Mesmo Assim" ao lado da mensagem sobre o YW.
   Ou, no Terminal:   xattr -dr com.apple.quarantine /Applications/YW.app
   Depois disso ele abre normalmente.
3. O macOS pede acesso ao microfone na primeira vez que você gravar.
EOF

rm -f "$OUTDIR/$NAME.dmg"
# hdiutil sometimes fails with "Resource busy" on CI runners (a transient lock on the image); retry a few times
for attempt in 1 2 3 4 5; do
    hdiutil create -volname "YW Studio $LABEL" -srcfolder "$STAGE" -fs HFS+ -format UDZO -imagekey zlib-level=9 -ov \
        "$OUTDIR/$NAME.dmg" && break
    [[ $attempt -lt 5 ]] || exit 1
    echo "hdiutil create failed (attempt $attempt), retrying"; sleep $((attempt * 10))
done
hdiutil verify "$OUTDIR/$NAME.dmg"
ls -la "$OUTDIR/$NAME.dmg"
