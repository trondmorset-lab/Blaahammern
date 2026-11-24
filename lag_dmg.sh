#!/bin/bash
# Lager en DMG-installasjonsfil for T1 revPRO: dist/T1revPRO-<versjon>-macOS.dmg
#
# Innhold i DMG-en:
#   Installer T1 revPRO.pkg     – installerer VST3 og AU for alle brukere
#   Manuell installasjon/       – pluginene + snarveier til plugin-mappene
#   Les meg.txt
# Logoen brukes som ikon for disken.
#
# Kjør ./bygg.sh først (bygger, tester og signerer ad hoc).
set -e
cd "$(dirname "$0")"

R=build/T1revPRO_artefacts/Release
VST3="$R/VST3/T1 revPRO.vst3"
AU="$R/AU/T1 revPRO.component"
[ -d "$VST3" ] && [ -d "$AU" ] || { echo "Finner ikke pluginene. Kjør ./bygg.sh først."; exit 1; }
codesign -v "$VST3" && codesign -v "$AU"

VERSION=$(sed -n 's/^project(T1revPRO VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt)
ID=no.morsetlabs.t1revpro
NAVN="T1 revPRO $VERSION"
TMP=build/dmg
UT="dist/T1revPRO-$VERSION-macOS.dmg"

rm -rf "$TMP"
mkdir -p "$TMP/pkg" "$TMP/innhold" dist

# ---------------------------------------------------------------- .pkg
# Én pakke per format, hver med sin installasjonsmappe (som i CI-workflowen)
lag_komponent () {   # $1 = bundle, $2 = navn, $3 = installasjonsmappe
    mkdir -p "$TMP/pkg/root-$2"
    ditto "$1" "$TMP/pkg/root-$2/$(basename "$1")"
    pkgbuild --analyze --root "$TMP/pkg/root-$2" "$TMP/pkg/$2.plist" > /dev/null
    # Hindre at installeren "flytter" pluginen til en gammel kopi et annet sted
    plutil -replace 0.BundleIsRelocatable -bool NO "$TMP/pkg/$2.plist"
    pkgbuild --quiet --root "$TMP/pkg/root-$2" --component-plist "$TMP/pkg/$2.plist" \
        --identifier "$ID.$2" --version "$VERSION" --install-location "$3" "$TMP/pkg/$2.pkg"
}
lag_komponent "$VST3" vst3 "/Library/Audio/Plug-Ins/VST3"
lag_komponent "$AU"   au   "/Library/Audio/Plug-Ins/Components"

productbuild --synthesize --package "$TMP/pkg/vst3.pkg" --package "$TMP/pkg/au.pkg" "$TMP/pkg/distribution.xml"
sed -i '' "s#<installer-gui-script minSpecVersion=\"[0-9]*\">#&<title>T1 revPRO $VERSION</title>#" "$TMP/pkg/distribution.xml"
productbuild --quiet --distribution "$TMP/pkg/distribution.xml" --package-path "$TMP/pkg" \
    "$TMP/innhold/Installer T1 revPRO.pkg"

# ---------------------------------------------------------------- manuell installasjon
M="$TMP/innhold/Manuell installasjon"
mkdir -p "$M"
ditto "$VST3" "$M/T1 revPRO.vst3"
ditto "$AU"   "$M/T1 revPRO.component"
ln -s /Library/Audio/Plug-Ins/VST3       "$M/VST3-mappe"
ln -s /Library/Audio/Plug-Ins/Components "$M/Components-mappe"

cat > "$TMP/innhold/Les meg.txt" <<EOF
T1 revPRO $VERSION – Morset Labs
Syntetisk salklang (VST3 og AU) for macOS 10.13 eller nyere,
Apple Silicon og Intel.

INSTALLERE
Dobbeltklikk «Installer T1 revPRO.pkg» og følg instruksjonene.
Pluginene legges i:
  /Library/Audio/Plug-Ins/VST3/T1 revPRO.vst3
  /Library/Audio/Plug-Ins/Components/T1 revPRO.component

Installeren er ikke signert med et Apple-utviklersertifikat. Hvis macOS
sier at den ikke kan åpnes: høyreklikk fila og velg «Åpne», eller gå til
Systeminnstillinger → Personvern og sikkerhet og trykk «Åpne likevel».

MANUELL INSTALLASJON
Åpne mappen «Manuell installasjon» og dra
  «T1 revPRO.vst3»      til «VST3-mappe»
  «T1 revPRO.component» til «Components-mappe»
(Finder spør om administratorpassord.) Vil du bare installere for din
egen bruker, legg dem i ~/Library/Audio/Plug-Ins/VST3 og .../Components.

ETTERPÅ
REAPER: Options → Preferences → Plug-ins → VST → «Re-scan».
Logic / GarageBand: start programmet på nytt.
Pluginen heter «Morset Labs: T1 revPRO».

AVINSTALLERE
Slett de to filene over.
EOF

# ---------------------------------------------------------------- diskikon fra logoen
IS="$TMP/T1revPRO.iconset"
mkdir -p "$IS"
for s in 16 32 128 256 512; do
    sips -z $s $s assets/t1revpro-logo-1024.png --out "$IS/icon_${s}x${s}.png" > /dev/null
    d=$((s * 2))
    sips -z $d $d assets/t1revpro-logo-1024.png --out "$IS/icon_${s}x${s}@2x.png" > /dev/null
done
iconutil -c icns "$IS" -o "$TMP/innhold/.VolumeIcon.icns"

# ---------------------------------------------------------------- DMG
hdiutil create -quiet -volname "$NAVN" -srcfolder "$TMP/innhold" -fs HFS+ -format UDRW -ov "$TMP/rw.dmg"
mkdir -p "$TMP/mnt"
hdiutil attach -quiet -nobrowse -noverify -noautoopen -mountpoint "$TMP/mnt" "$TMP/rw.dmg"
SetFile -a C "$TMP/mnt"            # bruk .VolumeIcon.icns som diskikon
SetFile -a V "$TMP/mnt/.VolumeIcon.icns"
sync
hdiutil detach -quiet "$TMP/mnt"
rm -f "$UT"
hdiutil convert -quiet "$TMP/rw.dmg" -format UDZO -imagekey zlib-level=9 -o "$UT"
hdiutil verify -quiet "$UT"

echo "Laget $UT ($(du -h "$UT" | cut -f1))"
