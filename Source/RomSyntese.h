#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <vector>

// Syntese av impulsrespons: spredte tidlige refleksjoner + frekvensavhengig,
// eksponentielt avtagende dekorrelert støy (samme metode som lag_klang.py).
namespace romsyntese
{
    struct RomOppsett
    {
        const char* navn;
        double erLengde;   // tidsvindu for tidlige refleksjoner (s)
        int erAntall;
        double erAndel;    // energi tidlige refleksjoner / hale
        double oppbygging; // myk innsvinging av halen (s)
        double demping;    // over denne dør diskanten raskere ut (Hz)
        double hp;         // low cut på halen (Hz)
        double lp;         // diskantkutt (Hz)
        double bassRT;     // RT-multiplikator under 250 Hz
        int seed;
    };

    const std::array<RomOppsett, 3>& rom();

    // RT-multiplikator for båndene (bass, lav mellom, mellom, høy mellom, diskant).
    struct Band { double lo, hi, mult; };
    std::array<Band, 5> band (const RomOppsett&);

    struct Refleksjon { double tid; float pan; float amp; float itd; };
    std::vector<Refleksjon> refleksjoner (const RomOppsett&, double predelay);

    // Stereo-IR ved samplerate fs. rt = RT60 i mellomtonen (s).
    // diffusion 0..1: 0 = glisne, hørbare ekko, 1 = helt tett og glatt.
    juce::AudioBuffer<float> lagIR (const RomOppsett&, double rt, double predelay,
                                   float tidligeDb, double diffusion, double fs);

    // Lengde på IR-en (s) for gitt RT og predelay.
    double irLengde (double rt, double predelay);
}
