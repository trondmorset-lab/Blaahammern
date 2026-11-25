#pragma once

#include <juce_core/juce_core.h>
#include <array>
#include <cmath>

// Modulasjon av klanghalen uten tonehøydeendring og uten «phaser».
//
// Halen finnes i tre varianter: samme rom, samme klangtid og spektrum, men
// med uavhengig støy (ukorrelerte). Modulasjonen glir sakte mellom dem med
// konstant effekt (sum av gain² = 1). Fordi variantene er ukorrelerte, flytter
// mønsteret av topper og daler seg tilfeldig over frekvensene, slik det gjør i
// en sal med luftbevegelse, i stedet for et jevnt kamfilter som sveiper (som
// forsinkelsesmodulasjon gir). Ved dybde 0 brukes bare variant 0, som er
// nøyaktig den syntetiske IR-en.
//
// Venstre og høyre har egne LFO-er (ujevne: to sinuser med ulike rater).
// Lydtråden: ingen allokering, ingen låser.
class Modulasjon
{
public:
    static constexpr int antall = 3;

    void prepare (double sampleRate)
    {
        fs = sampleRate;
        fase = startFase();
        dybde.reset (fs, 0.1);
        dybde.setCurrentAndTargetValue (0.0f);
    }

    // Legger sum_k g_k * hale[k] til ut (per kanal). dybde01 0..1.
    void bland (const std::array<const float*, antall>& haleL, const std::array<const float*, antall>& haleR,
                float* utL, float* utR, int n, float dybde01, float rateHz) noexcept
    {
        dybde.setTargetValue (juce::jlimit (0.0f, 1.0f, dybde01));
        const double tau = juce::MathConstants<double>::twoPi;
        const double grunn = tau * rateHz / fs;
        static constexpr double rateFaktor[2][2][2] = { { { 1.0, 2.37 }, { 0.71, 1.93 } },
                                                        { { 0.89, 2.11 }, { 0.63, 1.71 } } };
        for (int i = 0; i < n; ++i)
        {
            const float d = dybde.getNextValue();
            for (int ch = 0; ch < 2; ++ch)
            {
                // Variant 1 og 2 får hver sin ujevne LFO (±1).
                float u[2];
                for (int v = 0; v < 2; ++v)
                {
                    auto& f = fase[(size_t) ch][(size_t) v];
                    u[v] = (float) ((std::sin (f[0]) + 0.5 * std::sin (f[1])) / 1.5);
                    f[0] += grunn * rateFaktor[ch][v][0];
                    f[1] += grunn * rateFaktor[ch][v][1];
                    for (auto& x : f) if (x > tau * 1000.0) x -= tau * 1000.0;
                }
                const float g1 = d * u[0], g2 = d * u[1];
                const float norm = 1.0f / std::sqrt (1.0f + g1 * g1 + g2 * g2);
                const auto& h = ch == 0 ? haleL : haleR;
                float* ut = ch == 0 ? utL : utR;
                ut[i] += norm * (h[0][i] + g1 * h[1][i] + g2 * h[2][i]);
            }
        }
    }

private:
    using Faser = std::array<std::array<std::array<double, 2>, 2>, 2>; // [kanal][variant][sinus]
    static Faser startFase()
    {
        Faser f;
        f[0][0] = { 0.0, 1.1 }; f[0][1] = { 2.0, 0.4 };
        f[1][0] = { 1.3, 2.9 }; f[1][1] = { 3.7, 5.1 };
        return f;
    }

    double fs = 48000.0;
    Faser fase = startFase();
    juce::SmoothedValue<float> dybde;
};
