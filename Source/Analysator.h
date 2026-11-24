#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>

// Låsefri kø fra lydtråden til grensesnittet (mono-sum av L og R).
// push() kalles på lydtråden: ingen allokering, ingen låser.
class AnalyseFifo
{
public:
    void push (const float* l, const float* r, int n) noexcept;
    int pull (float* dest, int maks) noexcept;

private:
    static constexpr int storrelse = 1 << 15;
    juce::AbstractFifo fifo { storrelse };
    std::vector<float> buf = std::vector<float> ((size_t) storrelse);
};

// Spektrumanalysator for visning (kjøres på meldingstråden).
// 4096-punkts FFT med Hann-vindu, rask stigning og roligere fall.
class Spektrum
{
public:
    static constexpr int orden = 12, N = 1 << orden;

    Spektrum();
    void hent (AnalyseFifo&);           // tøm køen inn i historikken
    void beregn (double fs);            // ny FFT og glatting
    float db (double f) const;          // dBFS ved f, høyeste bin innen 1/12 oktav
    void nullstill();

private:
    juce::dsp::FFT fft { orden };
    std::vector<float> historikk, vindu, arbeid, glatt;
    int skrivPos = 0;
    double fs = 48000.0;
    float vinduSum = 1.0f;
};
