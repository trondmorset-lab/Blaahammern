#include "RomSyntese.h"
#include <cmath>

using namespace juce;

namespace romsyntese
{
namespace
{
    struct Biquad
    {
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;

        static Biquad lag (bool lavpass, double f, double fs, double q = 0.70710678)
        {
            f = jmin (f, fs * 0.45);
            const double w = MathConstants<double>::twoPi * f / fs;
            const double cw = std::cos (w), alpha = std::sin (w) / (2.0 * q);
            const double a0 = 1.0 + alpha;
            Biquad k;
            if (lavpass) { k.b0 = (1 - cw) / 2; k.b1 = 1 - cw;    k.b2 = (1 - cw) / 2; }
            else         { k.b0 = (1 + cw) / 2; k.b1 = -(1 + cw); k.b2 = (1 + cw) / 2; }
            k.a1 = -2 * cw; k.a2 = 1 - alpha;
            k.b0 /= a0; k.b1 /= a0; k.b2 /= a0; k.a1 /= a0; k.a2 /= a0;
            return k;
        }

        // Peaking-EQ (RBJ).
        static Biquad topp (double f, double q, double gainDb, double fs)
        {
            const double A = std::pow (10.0, gainDb / 40.0);
            const double w = MathConstants<double>::twoPi * jmin (f, fs * 0.45) / fs;
            const double cw = std::cos (w), alpha = std::sin (w) / (2.0 * q);
            const double a0 = 1.0 + alpha / A;
            Biquad k;
            k.b0 = (1 + alpha * A) / a0; k.b1 = -2 * cw / a0; k.b2 = (1 - alpha * A) / a0;
            k.a1 = -2 * cw / a0;         k.a2 = (1 - alpha / A) / a0;
            return k;
        }

        void kjor (std::vector<float>& x) const
        {
            double x1 = 0, x2 = 0, y1 = 0, y2 = 0;
            for (auto& s : x)
            {
                const double xi = s;
                const double yi = b0 * xi + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
                x2 = x1; x1 = xi; y2 = y1; y1 = yi;
                s = (float) yi;
            }
        }
    };

    void lp4 (std::vector<float>& x, double f, double fs) { auto k = Biquad::lag (true, f, fs);  k.kjor (x); k.kjor (x); }
    void hp4 (std::vector<float>& x, double f, double fs) { auto k = Biquad::lag (false, f, fs); k.kjor (x); k.kjor (x); }

    double energi (const std::vector<float>& x)
    {
        double e = 0;
        for (auto s : x) e += (double) s * s;
        return e;
    }

    float gauss (Random& r)
    {
        const double u1 = jmax (1.0e-12, (double) r.nextDouble());
        const double u2 = r.nextDouble();
        return (float) (std::sqrt (-2.0 * std::log (u1)) * std::cos (MathConstants<double>::twoPi * u2));
    }
}

const std::array<RomOppsett, 3>& rom()
{
    static const std::array<RomOppsett, 3> r {{
        { "Concert Hall", 0.085, 22, 0.22, 0.050, 7500.0, 170.0, 11000.0, 1.05, 11 },
        { "Chamber",     0.060, 26, 0.32, 0.035, 8000.0, 190.0, 12000.0, 1.00, 23 },
        { "Warm Hall",   0.100, 20, 0.18, 0.060, 6000.0, 150.0,  9500.0, 1.12, 37 },
    }};
    return r;
}

std::array<Band, antallBand> band (const RomOppsett& r)
{
    return {{ { 0.0, 250.0, r.bassRT },
              { 250.0, 1000.0, 1.03 },
              { 1000.0, 3500.0, 1.0 },
              { 3500.0, r.demping, 0.78 },
              { r.demping, 0.0, 0.45 } }};
}

std::vector<Refleksjon> refleksjoner (const RomOppsett& r, double predelay)
{
    Random rng (r.seed);
    std::vector<Refleksjon> ut;
    for (int i = 0; i < r.erAntall; ++i)
    {
        const double u = (i + rng.nextDouble() * 0.8) / r.erAntall;
        Refleksjon f;
        f.tid = predelay + r.erLengde * std::pow (u, 1.4);
        f.pan = (float) (0.08 + rng.nextDouble() * (MathConstants<double>::halfPi - 0.16));
        f.amp = (float) ((0.55 + 0.45 * rng.nextDouble()) * (1.0 - 0.6 * u));
        if (rng.nextInt (4) == 0) f.amp = -f.amp;
        f.itd = (float) ((rng.nextDouble() * 2.0 - 1.0) * 0.0006);
        ut.push_back (f);
    }
    return ut;
}

double irLengde (double rt, double predelay)
{
    return predelay + rt * 1.25 + 0.05;
}

AudioBuffer<float> lagIR (const RomOppsett& r, double rt, double predelay, float tidligeDb,
                         double diffusion, double fs, int variant)
{
    const double d = jlimit (0.0, 1.0, diffusion);
    const int n = (int) (irLengde (rt, predelay) * fs);
    const int haleStart = (int) ((predelay + 0.006) * fs);
    // Lav diffusion: raskere oppbygging og glissen hale (hørbare enkeltekko).
    const int bygg = jmax (1, (int) (r.oppbygging * (0.3 + 0.7 * d) * fs));
    // Tetthet: 150 ekko/s ved 0 %, helt tett støy ved 100 %.
    const double p = std::pow (150.0 / fs, 1.0 - d);
    const float pSkala = (float) (1.0 / std::sqrt (p));
    const auto refl = refleksjoner (r, predelay);
    const auto baand = band (r);

    std::array<std::vector<float>, 2> hale, er;
    for (int ch = 0; ch < 2; ++ch)
    {
        // Uavhengig, fast støy per kanal og rom: dekorrelert stereo, og
        // samme "rom" uansett lengde.
        Random rng (r.seed * 7919 + ch * 104729 + 1 + variant * 1000003);
        std::vector<float> stoy ((size_t) n);
        for (auto& s : stoy)
        {
            const float g = gauss (rng);
            s = (p >= 1.0 || rng.nextDouble() < p) ? g * pSkala : 0.0f;
        }

        auto& h = hale[(size_t) ch];
        h.assign ((size_t) n, 0.0f);
        for (const auto& b : baand)
        {
            std::vector<float> s (stoy);
            if (b.lo > 0) hp4 (s, b.lo, fs);
            if (b.hi > 0) lp4 (s, b.hi, fs);
            const double k = 6.907755 / (rt * b.mult * fs);
            for (int i = haleStart; i < n; ++i)
            {
                const int t = i - haleStart;
                double env = std::exp (-k * t);
                if (t < bygg)
                    env *= 0.5 - 0.5 * std::cos (MathConstants<double>::pi * t / bygg);
                h[(size_t) i] += (float) (s[(size_t) i] * env);
            }
        }
        hp4 (h, r.hp, fs);
        Biquad::lag (true, r.lp, fs, 0.55).kjor (h);
        // Myk senkning rundt 950 Hz (700-1200 Hz) i halen, så den ikke blir
        // hard i mellomtonen: fast midtDipDb, og i tillegg midtDipSenDb som
        // fases inn over halen (mellomtonen dør litt raskere ut).
        {
            std::vector<float> dyp (h);
            Biquad::topp (midtDipHz, 0.8, midtDipDb, fs).kjor (h);
            Biquad::topp (midtDipHz, 0.8, midtDipDb + midtDipSenDb, fs).kjor (dyp);
            const double tau = 0.6 * rt * fs;
            for (int i = haleStart; i < n; ++i)
            {
                const float w = (float) jmin (1.0, (i - haleStart) / tau);
                h[(size_t) i] += w * (dyp[(size_t) i] - h[(size_t) i]);
            }
        }

        // Tidlige refleksjoner: korte, myke pulser med panorering.
        auto& e = er[(size_t) ch];
        e.assign ((size_t) n, 0.0f);
        const int pl = jmax (3, (int) std::round (6.0 * fs / 48000.0));
        // Diffusion smører hver refleksjon ut i en kort støysky (opptil 6 ms).
        const int sky = (int) (d * 0.006 * fs);
        Random skyRng (r.seed * 31 + ch);
        for (const auto& f : refl)
        {
            const float gs = (ch == 0 ? std::cos (f.pan) : std::sin (f.pan)) * f.amp;
            const int i0 = (int) ((f.tid + (ch == 1 ? f.itd : 0.0)) * fs);
            for (int j = 0; j < sky; ++j)
                if (i0 + j >= 0 && i0 + j < n)
                    e[(size_t) (i0 + j)] += gs * (float) d * 0.45f * gauss (skyRng)
                                            * std::exp (-3.0f * (float) j / (float) sky);
            const float g = ch == 0 ? std::cos (f.pan) : std::sin (f.pan);
            const double t = f.tid + (ch == 1 ? f.itd : 0.0);
            const int idx = (int) (t * fs);
            for (int j = 0; j <= pl; ++j)
                if (idx + j >= 0 && idx + j < n)
                    e[(size_t) (idx + j)] += f.amp * g * (1.0f - 0.55f * (float) d) * (0.5f - 0.5f * std::cos (MathConstants<float>::twoPi * (float) j / (float) pl));
        }
        hp4 (e, 120.0, fs);
        lp4 (e, 9000.0, fs);
        Biquad::topp (midtDipHz, 0.8, midtDipDb * 0.5, fs).kjor (e);
    }

    const double eh = energi (hale[0]) + energi (hale[1]);
    const double ee = energi (er[0]) + energi (er[1]);
    const float gEr = (float) (std::sqrt (r.erAndel * eh / jmax (ee, 1e-20)) * Decibels::decibelsToGain ((double) tidligeDb, -100.0));

    AudioBuffer<float> ir (2, n);
    const int f0 = (int) (n * 0.88);
    for (int ch = 0; ch < 2; ++ch)
    {
        float* d = ir.getWritePointer (ch);
        for (int i = 0; i < n; ++i)
        {
            float v = hale[(size_t) ch][(size_t) i] + gEr * er[(size_t) ch][(size_t) i];
            if (i > f0)
                v *= 0.5f + 0.5f * std::cos (MathConstants<float>::pi * (float) (i - f0) / (float) (n - f0));
            d[i] = v;
        }
    }

    // Nivå: halen starter like sterkt uansett lengde, slik at lengre klang
    // også gir mer klang (energi ∝ RT). Ved 2,4 s ca. -6 dB under tørt.
    double eMaks = 0;
    for (int ch = 0; ch < 2; ++ch)
    {
        double e = 0;
        for (int i = 0; i < n; ++i) e += (double) ir.getSample (ch, i) * ir.getSample (ch, i);
        eMaks = jmax (eMaks, e);
    }
    const double maal = 0.25 * (rt / 2.4);
    ir.applyGain ((float) std::sqrt (maal / jmax (eMaks, 1e-20)));
    return ir;
}
}
