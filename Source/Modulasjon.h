#pragma once

#include <juce_core/juce_core.h>
#include <array>
#include <cmath>
#include <vector>

// Modulasjon av klanghalen: to svakt modulerte forsinkelser per kanal
// (vibrato/chorus som i algoritmiske haller). Hver tapp har egen, ujevn LFO
// (to sinuser med ulike rater), og venstre og høyre er forskjellige. Halen
// blir dermed levende og bredere i stedet for statisk.
//
// Basisforsinkelsen (basis) er alltid med, også ved dybde 0. Prosessoren
// trekker den fra i IR-en, så den totale tiden stemmer.
// Lydtråden: ingen allokering etter prepare(), ingen låser.
class Modulasjon
{
public:
    static constexpr double basis = 0.005;     // s
    static constexpr double maksDybde = 0.0010; // s ved 100 %

    void prepare (double sampleRate)
    {
        fs = sampleRate;
        const int trengs = (int) std::ceil ((basis + maksDybde) * fs) + 8;
        int storrelse = 1;
        while (storrelse < trengs) storrelse <<= 1;
        maske = storrelse - 1;
        for (auto& b : buf) b.assign ((size_t) storrelse, 0.0f);
        skriv = 0;
        for (int ch = 0; ch < 2; ++ch)
            for (int t = 0; t < 2; ++t)
                fase[(size_t) ch][(size_t) t] = startFase[ch][t];
        dybde.reset (fs, 0.1);
        dybde.setCurrentAndTargetValue (0.0f);
    }

    int basisSamples() const { return (int) std::round (basis * fs); }

    // dybde01 0..1, rateHz = LFO-grunnrate.
    void process (float* l, float* r, int n, float dybde01, float rateHz) noexcept
    {
        dybde.setTargetValue (juce::jlimit (0.0f, 1.0f, dybde01));
        const double basisS = (double) basisSamples(); // samme avrunding som i IR-en
        const double dybdeS = maksDybde * fs;
        const double tau = juce::MathConstants<double>::twoPi;
        std::array<double, 4> steg;
        for (int ch = 0; ch < 2; ++ch)
            for (int t = 0; t < 2; ++t)
                steg[(size_t) (ch * 2 + t)] = tau * rateHz * rateFaktor[ch][t] / fs;

        for (int i = 0; i < n; ++i)
        {
            const float dy = dybde.getNextValue();
            const double d = dy * dybdeS;
            // To tapper i ulik fase summeres ikke lenger i fase når dybden
            // øker; løft nivået litt så halen er like sterk.
            const float g = 0.5f * (1.0f + 0.38f * dy);
            float* x[2] = { l, r };
            for (int ch = 0; ch < 2; ++ch)
            {
                auto& b = buf[(size_t) ch];
                b[(size_t) (skriv & maske)] = x[ch][i];
                float sum = 0.0f;
                for (int t = 0; t < 2; ++t)
                {
                    double& f = fase[(size_t) ch][(size_t) t];
                    const double lfo = (std::sin (f) + 0.3 * std::sin (1.93 * f + 1.3 * (ch + 2 * t))) / 1.3;
                    f += steg[(size_t) (ch * 2 + t)];
                    if (f > tau * 100.0) f -= tau * 100.0;
                    sum += les (b, (double) skriv - (basisS + d * lfo));
                }
                x[ch][i] = g * sum;
            }
            ++skriv;
        }
        skriv &= maske;
    }

private:
    // 4-punkts Hermite-interpolasjon (taper lite diskant ved modulasjon).
    float les (const std::vector<float>& b, double pos) const noexcept
    {
        const double gulv = std::floor (pos);
        const int i = (int) gulv;
        const float fr = (float) (pos - gulv);
        const float y0 = b[(size_t) ((i - 1) & maske)], y1 = b[(size_t) (i & maske)];
        const float y2 = b[(size_t) ((i + 1) & maske)], y3 = b[(size_t) ((i + 2) & maske)];
        const float c1 = 0.5f * (y2 - y0);
        const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * fr + c2) * fr + c1) * fr + y1;
    }

    static constexpr double rateFaktor[2][2] = { { 1.0, 1.31 }, { 0.87, 1.17 } };
    static constexpr double startFase[2][2]  = { { 0.0, 2.1 }, { 1.0, 3.4 } };

    double fs = 48000.0;
    std::array<std::vector<float>, 2> buf;
    int maske = 0, skriv = 0;
    std::array<std::array<double, 2>, 2> fase {};
    juce::SmoothedValue<float> dybde;
};
