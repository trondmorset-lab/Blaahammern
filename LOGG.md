# Arbeidslogg – T1 revPRO

Nyeste øverst. Hver oppføring: hva som er gjort, beslutninger, prøvd og forkastet, og erfaringer.

## 1.10.0 – Above/Below på dynamisk EQ, versjon i GUI
- **Gjort:** parameteren `dynN_mode` (Choice «Above»/«Below», versjon 6). BELOW demper når RMS i
  båndet er *under* Thresh (nedover-ekspansjon), med samme knekk, ratio og Range som ABOVE.
  Knappene «1 ABOVE» / «2 BELOW» står øverst til høyre i DYNAMIC EQ og vises bare i DYN EQ-fanen.
  Overskriften i visningen og lyset på nivåstreken følger modusen. Versjonen («v1.10.0») vises
  under navnet, fra makroen `T1REVPRO_VERSION` som CMake setter fra `project(VERSION)`.
- **Tester:** 4 nye DSP-tester for BELOW (svakt signal dempes 12 dB, sterkt uendret) og
  3 GUI-tester for knappen. Alle består, AU-validering OK.
- **Erfaring:** `JucePlugin_VersionString` finnes ikke i testprogrammet (konsollapp), så egen
  makro er nødvendig når samme kildekode bygges i begge.

## 1.9.0 – modulasjon uten phaser
- **Beslutning:** halen krysstones med konstant effekt mellom tre ukorrelerte varianter av samme IR.
- **Prøvd og forkastet:** forsinkelsesmodulasjon (1.7/1.8). Modulert hale summert med korrelert
  umodulert innhold gir et kamfilter som sveiper, altså phaserlyd. Én tapp per kanal svingte
  fortsatt 5,7 dB i spekteret.
- **Erfaring:** test «ingen tonehøydeendring» med en sinus (≥ 99,5 % energi innen ±10 Hz).
  CPU ca. 5,5 % av sanntid med tre halekonvolusjoner.

## 1.8.0 – roligere modulasjon, tørrere mix
- Mix-kurve: tørt = min(1, 2(1−p)), vått = min(1, 2p)^1,7 (25 % → −10 dB). DryWetMixer fjernet.

## 1.6.0–1.7.0 – mykere mellomtone, dynamisk EQ, dra-punkter, modulasjon
- **Prøvd og forkastet:** et eget decay-bånd for 700–1300 Hz. LR4-delefiltrene overlapper og ga
  6 dB dipp. Bruk heller en peaking-dipp (950 Hz) som fases inn over halen.
- **Erfaring:** pushen kan avvises når remote har README-endringer fra nettet. Rebase og flett
  norsk tekst inn i README.no.md.
