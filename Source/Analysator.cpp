#include "Analysator.h"
#include <cmath>

using namespace juce;

void AnalyseFifo::push (const float* l, const float* r, int n) noexcept
{
    int s1, b1, s2, b2;
    fifo.prepareToWrite (n, s1, b1, s2, b2);
    for (int i = 0; i < b1; ++i) buf[(size_t) (s1 + i)] = 0.5f * (l[i] + r[i]);
    for (int i = 0; i < b2; ++i) buf[(size_t) (s2 + i)] = 0.5f * (l[b1 + i] + r[b1 + i]);
    fifo.finishedWrite (b1 + b2); // full kø: resten droppes (bare visning)
}

int AnalyseFifo::pull (float* dest, int maks) noexcept
{
    int s1, b1, s2, b2;
    fifo.prepareToRead (maks, s1, b1, s2, b2);
    if (b1 > 0) std::copy (buf.begin() + s1, buf.begin() + s1 + b1, dest);
    if (b2 > 0) std::copy (buf.begin() + s2, buf.begin() + s2 + b2, dest + b1);
    fifo.finishedRead (b1 + b2);
    return b1 + b2;
}

Spektrum::Spektrum()
    : historikk ((size_t) N, 0.0f), vindu ((size_t) N), arbeid ((size_t) (2 * N), 0.0f),
      glatt ((size_t) (N / 2 + 1), -120.0f)
{
    vinduSum = 0.0f;
    for (int i = 0; i < N; ++i)
    {
        vindu[(size_t) i] = 0.5f - 0.5f * std::cos (MathConstants<float>::twoPi * (float) i / (float) N);
        vinduSum += vindu[(size_t) i];
    }
}

void Spektrum::nullstill()
{
    std::fill (historikk.begin(), historikk.end(), 0.0f);
    std::fill (glatt.begin(), glatt.end(), -120.0f);
}

void Spektrum::hent (AnalyseFifo& kilde)
{
    float tmp[1024];
    for (;;)
    {
        const int n = kilde.pull (tmp, 1024);
        if (n == 0) break;
        for (int i = 0; i < n; ++i)
        {
            historikk[(size_t) skrivPos] = tmp[i];
            skrivPos = (skrivPos + 1) % N;
        }
    }
}

void Spektrum::beregn (double samplerate)
{
    fs = samplerate;
    for (int i = 0; i < N; ++i)
        arbeid[(size_t) i] = historikk[(size_t) ((skrivPos + i) % N)] * vindu[(size_t) i];
    std::fill (arbeid.begin() + N, arbeid.end(), 0.0f);
    fft.performFrequencyOnlyForwardTransform (arbeid.data(), true);

    // Skalert slik at en sinus med amplitude A gir 20*log10(A) dBFS.
    const float skala = 2.0f / vinduSum;
    for (int k = 0; k <= N / 2; ++k)
    {
        const float ny = Decibels::gainToDecibels (arbeid[(size_t) k] * skala, -120.0f);
        float& g = glatt[(size_t) k];
        g += (ny - g) * (ny > g ? 0.6f : 0.12f); // rask stigning, rolig fall
    }
}

float Spektrum::db (double f) const
{
    const double binHz = fs / N;
    const double lo = f * std::pow (2.0, -1.0 / 24.0) / binHz;
    const double hi = f * std::pow (2.0, 1.0 / 24.0) / binHz;
    const int kLo = jlimit (1, N / 2, (int) std::ceil (lo));
    const int kHi = jlimit (1, N / 2, (int) std::floor (hi));
    if (kHi >= kLo)
    {
        // Høyeste bin i 1/12-oktavbåndet (som de fleste analysatorer):
        // toner vises med riktig nivå i dBFS.
        float maks = -120.0f;
        for (int k = kLo; k <= kHi; ++k)
            maks = jmax (maks, glatt[(size_t) k]);
        return maks;
    }
    // Under ett bin bredt: lineær interpolasjon mellom nabobinene.
    const double pos = jlimit (1.0, (double) (N / 2 - 1), f / binHz);
    const int k = (int) pos;
    const float t = (float) (pos - k);
    return glatt[(size_t) k] * (1.0f - t) + glatt[(size_t) (k + 1)] * t;
}
