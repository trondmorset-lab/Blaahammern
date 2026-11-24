#!/bin/bash
# Bygger T1 revPRO (VST3 + AU, universal), kjører testen og installerer i
# brukerens plugin-mapper. Bruker den lokale JUCE-kopien i ./JUCE hvis den finnes.
set -e
cd "$(dirname "$0")"
export PATH="$HOME/Library/Python/3.9/bin:$PATH"
JUCE_ARG=""
[ -d JUCE ] && JUCE_ARG="-DJUCE_DIR=$PWD/JUCE"
cmake -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DT1REVPRO_BUILD_TESTS=ON $JUCE_ARG > /dev/null
if ! cmake --build build -j8 > build/bygglogg.txt 2>&1; then
    grep -E "error" -A3 build/bygglogg.txt | head -40
    echo "BYGGET FEILET – ingenting er installert. Se build/bygglogg.txt"
    exit 1
fi
build/T1revPROTest_artefacts/Release/T1revPROTest .
R=build/T1revPRO_artefacts/Release
# JUCE legger moduleinfo.json i VST3-pakken etter signering; signer på nytt (ad hoc).
codesign --force --deep --sign - "$R/VST3/T1 revPRO.vst3" "$R/AU/T1 revPRO.component" 2>/dev/null
codesign -v "$R/VST3/T1 revPRO.vst3" && codesign -v "$R/AU/T1 revPRO.component"
mkdir -p ~/Library/Audio/Plug-Ins/VST3 ~/Library/Audio/Plug-Ins/Components
# fjern eldre versjoner (også det gamle navnet Fiolinklang)
rm -rf ~/Library/Audio/Plug-Ins/VST3/Fiolinklang.vst3 ~/Library/Audio/Plug-Ins/Components/Fiolinklang.component
rm -rf ~/Library/Audio/Plug-Ins/VST3/"T1 revPRO.vst3" ~/Library/Audio/Plug-Ins/Components/"T1 revPRO.component"
ditto "$R/VST3/T1 revPRO.vst3" ~/Library/Audio/Plug-Ins/VST3/"T1 revPRO.vst3"
ditto "$R/AU/T1 revPRO.component" ~/Library/Audio/Plug-Ins/Components/"T1 revPRO.component"
killall -9 AudioComponentRegistrar 2>/dev/null || true
echo "Installert i ~/Library/Audio/Plug-Ins/ (VST3 og Components)."
