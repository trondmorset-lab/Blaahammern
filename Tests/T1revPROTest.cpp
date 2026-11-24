// Kjører FiolinProcessor uten DAW og sjekker grunnleggende oppførsel.
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <juce_events/juce_events.h>
#include <cstdio>

using namespace juce;

static int feil = 0;
static void sjekk (bool ok, const char* hva, double verdi = 0.0)
{
    std::printf ("%s  %s (%.4g)\n", ok ? "OK  " : "FEIL", hva, verdi);
    if (! ok) ++feil;
}

static void sett (FiolinProcessor& p, const char* id, float verdi)
{
    auto* par = p.parametre.getParameter (id);
    par->setValueNotifyingHost (par->convertTo0to1 (verdi));
}

// Kjør en impuls gjennom og returner utgangen.
static AudioBuffer<float> impulsrespons (FiolinProcessor& p, double rate, double sek)
{
    const int blokk = 512;
    p.prepareToPlay (rate, blokk);
    MidiBuffer midi;
    AudioBuffer<float> b (2, blokk);
    for (int i = 0; i < 40; ++i) // la bakgrunnstråden i Convolution bytte inn IR-en
    {
        b.clear();
        p.processBlock (b, midi);
        Thread::sleep (10);
    }
    const int n = (int) (sek * rate);
    AudioBuffer<float> ut (2, n);
    for (int pos = 0; pos < n; pos += blokk)
    {
        const int len = jmin (blokk, n - pos);
        AudioBuffer<float> bb (2, len);
        bb.clear();
        if (pos == 0) { bb.setSample (0, 0, 1.0f); bb.setSample (1, 0, 1.0f); }
        p.processBlock (bb, midi);
        for (int ch = 0; ch < 2; ++ch)
            ut.copyFrom (ch, pos, bb, ch, 0, len);
    }
    return ut;
}

static double energi (const AudioBuffer<float>& b, double rate, double fra, double til)
{
    double e = 0.0;
    const int a = (int) (fra * rate), z = jmin (b.getNumSamples(), (int) (til * rate));
    for (int ch = 0; ch < 2; ++ch)
        for (int i = a; i < z; ++i)
            e += (double) b.getSample (ch, i) * b.getSample (ch, i);
    return e;
}

// T20 (-5 til -25 dB) fra Schroeder-integrasjon, ekstrapolert til 60 dB.
static double maalRT (const AudioBuffer<float>& b, double rate)
{
    const int n = b.getNumSamples();
    std::vector<double> edc ((size_t) n);
    double acc = 0;
    for (int i = n - 1; i >= 0; --i)
    {
        acc += (double) b.getSample (0, i) * b.getSample (0, i) + (double) b.getSample (1, i) * b.getSample (1, i);
        edc[(size_t) i] = acc;
    }
    int i5 = -1, i25 = -1;
    for (int i = 0; i < n; ++i)
    {
        const double db = 10 * std::log10 (edc[(size_t) i] / edc[0] + 1e-30);
        if (i5 < 0 && db <= -5) i5 = i;
        if (i25 < 0 && db <= -25) { i25 = i; break; }
    }
    return (i5 < 0 || i25 < 0) ? 0.0 : 3.0 * (i25 - i5) / rate;
}

static bool endelig (const AudioBuffer<float>& b)
{
    for (int ch = 0; ch < b.getNumChannels(); ++ch)
        for (int i = 0; i < b.getNumSamples(); ++i)
            if (! std::isfinite (b.getSample (ch, i))) return false;
    return true;
}

static int forsteLyd (const AudioBuffer<float>& b)
{
    for (int i = 0; i < b.getNumSamples(); ++i)
        if (std::abs (b.getSample (0, i)) > 1e-3f || std::abs (b.getSample (1, i)) > 1e-3f) return i;
    return -1;
}

int main (int argc, char** argv)
{
    ScopedJuceInitialiser_GUI init;
    // Skjermbilder skrives til mappen i første argument (standard: gjeldende mappe).
    const File utMappe = argc > 1 ? File::getCurrentWorkingDirectory().getChildFile (argv[1])
                                  : File::getCurrentWorkingDirectory();
    FiolinProcessor p;

    // 1. Standard (Konsertsal, 2,4 s, 25 ms), 100 % våt.
    sett (p, "mix", 100.0f);
    auto ir = impulsrespons (p, 48000.0, 5.0);
    sjekk (endelig (ir), "ingen NaN/inf");
    const double e0 = energi (ir, 48000, 0.0, 5.0);
    sjekk (e0 > 0.1 && e0 < 2.0, "våt energi rimelig (ca. -6 dB per kanal)", e0);
    const double rt24 = maalRT (ir, 48000);
    sjekk (std::abs (rt24 - 2.4) < 0.35, "målt RT ved lengde 2,4 s", rt24);
    sjekk (energi (ir, 48000, 3.5, 5.0) / e0 < 1e-6, "stille etter 3,5 s", energi (ir, 48000, 3.5, 5.0) / e0);
    {
        double lr = 0, ll = 0, rr = 0;
        for (int i = 4800; i < 96000; ++i)
        {
            const double l = ir.getSample (0, i), r = ir.getSample (1, i);
            lr += l * r; ll += l * l; rr += r * r;
        }
        const double k = lr / std::sqrt (ll * rr);
        sjekk (std::abs (k) < 0.3, "dekorrelert stereo", k);
    }
    sjekk (std::abs (forsteLyd (ir) / 48.0 - 25.0) < 3.0, "predelay 25 ms", forsteLyd (ir) / 48.0);

    // 2. Lengde i sekunder: 1,2 og 4,5 s.
    sett (p, "lengde_s", 1.2f);
    auto kort = impulsrespons (p, 48000.0, 3.0);
    const double rt12 = maalRT (kort, 48000);
    sjekk (std::abs (rt12 - 1.2) < 0.2, "målt RT ved lengde 1,2 s", rt12);
    sett (p, "lengde_s", 4.5f);
    auto lang = impulsrespons (p, 48000.0, 7.0);
    const double rt45 = maalRT (lang, 48000);
    sjekk (std::abs (rt45 - 4.5) < 0.6, "målt RT ved lengde 4,5 s", rt45);
    sjekk (energi (lang, 48000, 0, 7) > energi (kort, 48000, 0, 3), "lengre klang gir mer klang");
    sett (p, "lengde_s", 10.0f);
    auto tisek = impulsrespons (p, 48000.0, 13.0);
    sjekk (endelig (tisek), "10 s: ingen NaN/inf");
    sjekk (std::abs (maalRT (tisek, 48000) - 10.0) < 1.3, "målt RT ved lengde 10,0 s", maalRT (tisek, 48000));
    {
        auto* par = p.parametre.getParameter ("lengde_s");
        sjekk (std::abs (par->convertFrom0to1 (1.0f) - 10.0f) < 1e-4f, "maks lengde 10,0 s", par->convertFrom0to1 (1.0f));
    }
    {
        auto* par = p.parametre.getParameter ("lengde_s");
        sett (p, "lengde_s", 2.43f);
        const float v = par->convertFrom0to1 (par->getValue());
        sjekk (std::abs (v - 2.4f) < 1e-3f, "lengde rundes til 0,1 s", v);
        sjekk (par->getCurrentValueAsText() == "2.4 s", "decay vises som «2.4 s» (engelsk)");
        sjekk (std::abs (par->getValueForText ("3,7") - par->convertTo0to1 (3.7f)) < 1e-4f, "kan skrive «3,7»");
    }

    // 2b. Diffusion: lav gir glissen, «kornete» hale (høy kurtosis), høy gir tett.
    {
        sett (p, "lengde_s", 2.4f);
        auto kurtosis = [] (const AudioBuffer<float>& b)
        {
            // normalisert med lokal RMS (vinduer på 10 ms) så avtakingen ikke teller
            double sum = 0; int n = 0;
            for (int start = 14400; start + 480 <= 38400; start += 480)
            {
                double e2 = 0, e4 = 0;
                for (int i = start; i < start + 480; ++i) { const double v = b.getSample (0, i); e2 += v * v; e4 += v * v * v * v; }
                e2 /= 480; e4 /= 480;
                if (e2 > 0) { sum += e4 / (e2 * e2); ++n; }
            }
            return n > 0 ? sum / n : 0.0;
        };
        sett (p, "diffusion", 0.0f);
        auto glissen = impulsrespons (p, 48000.0, 1.0);
        sett (p, "diffusion", 100.0f);
        auto tett = impulsrespons (p, 48000.0, 1.0);
        sett (p, "diffusion", 80.0f);
        sjekk (kurtosis (glissen) > 6.0, "diffusion 0 %: glissen hale", kurtosis (glissen));
        sjekk (kurtosis (tett) < 4.5, "diffusion 100 %: tett hale", kurtosis (tett));
        sjekk (endelig (glissen) && energi (glissen, 48000, 0, 1) > 0.05, "diffusion 0 %: gir lyd", energi (glissen, 48000, 0, 1));
    }

    // 2c. EQ: high shelf -12 dB demper diskanten i klangen; responsfunksjonen stemmer.
    {
        sjekk (std::abs (FiolinProcessor::eqRespons (p.parametre, 1000.0)) < 0.5, "EQ flat som standard", FiolinProcessor::eqRespons (p.parametre, 1000.0));
        sett (p, "hishelf_gain", -12.0f);
        sett (p, "hishelf_freq", 4000.0f);
        sjekk (std::abs (FiolinProcessor::eqRespons (p.parametre, 16000.0) + 12.0) < 1.0, "EQ-kurve: -12 dB ved 16 kHz", FiolinProcessor::eqRespons (p.parametre, 16000.0));
        auto mork = impulsrespons (p, 48000.0, 2.0);
        sett (p, "hishelf_gain", 0.0f);
        auto lysIR = impulsrespons (p, 48000.0, 2.0);
        auto hoyEnergi = [] (const AudioBuffer<float>& b)
        {
            double e = 0;   // energi i første differanse ~ diskant
            for (int i = 1; i < b.getNumSamples(); ++i) { const double d = b.getSample (0, i) - b.getSample (0, i - 1); e += d * d; }
            return e;
        };
        sjekk (hoyEnergi (mork) < 0.4 * hoyEnergi (lysIR), "high shelf -12 dB demper diskant", hoyEnergi (mork) / hoyEnergi (lysIR));
        sett (p, "loshelf_gain", 6.0f);
        sjekk (std::abs (FiolinProcessor::eqRespons (p.parametre, 40.0) - 6.0) < 1.0, "EQ-kurve: +6 dB low shelf", FiolinProcessor::eqRespons (p.parametre, 40.0));
        sett (p, "loshelf_gain", 0.0f);
        sett (p, "lavkutt", 200.0f);
        sjekk (FiolinProcessor::eqRespons (p.parametre, 50.0) < -20.0f, "EQ-kurve: low cut 200 Hz", FiolinProcessor::eqRespons (p.parametre, 50.0));
        sjekk (p.parametre.getParameter ("hoykutt")->getCurrentValueAsText() == "Off", "high cut viser «Off»");
        sett (p, "lavkutt", 20.0f);
    }

    // 3. Predelay 80 ms.
    sett (p, "predelay_ms", 80.0f);
    auto pre = impulsrespons (p, 48000.0, 1.0);
    sjekk (std::abs (forsteLyd (pre) / 48.0 - 80.0) < 3.0, "predelay 80 ms", forsteLyd (pre) / 48.0);
    sett (p, "predelay_ms", 25.0f);

    // 4. Romskifte og tidlige refleksjoner.
    sett (p, "rom", 2.0f);
    auto varm = impulsrespons (p, 48000.0, 3.0);
    double diff = 0;
    for (int i = 0; i < 48000; ++i) diff += std::abs (varm.getSample (0, i) - ir.getSample (0, i));
    sjekk (diff > 1.0, "romskifte virker", diff);
    sett (p, "tidlige", -24.0f);
    auto lite = impulsrespons (p, 48000.0, 3.0);
    sjekk (energi (lite, 48000, 0.02, 0.09) < 0.8 * energi (varm, 48000, 0.02, 0.09),
           "mindre tidlige refleksjoner", energi (lite, 48000, 0.02, 0.09) / energi (varm, 48000, 0.02, 0.09));
    sett (p, "tidlige", 0.0f);

    // 5. Bakgrunnstråden lager ny IR når parametre endres (som når knappen vris).
    sett (p, "lengde_s", 1.0f);
    p.oppdaterIR();
    Thread::sleep (400);
    {
        MidiBuffer midi;
        AudioBuffer<float> b (2, 512);
        for (int i = 0; i < 20; ++i) { b.clear(); p.processBlock (b, midi); Thread::sleep (5); }
        const int n = 48000 * 3;
        AudioBuffer<float> ut (2, n);
        for (int pos = 0; pos < n; pos += 512)
        {
            AudioBuffer<float> bb (2, 512);
            bb.clear();
            if (pos == 0) { bb.setSample (0, 0, 1.0f); bb.setSample (1, 0, 1.0f); }
            p.processBlock (bb, midi);
            for (int ch = 0; ch < 2; ++ch) ut.copyFrom (ch, pos, bb, ch, 0, jmin (512, n - pos));
        }
        const double rt = maalRT (ut, 48000);
        sjekk (std::abs (rt - 1.0) < 0.2, "bakgrunnstråd: ny lengde 1,0 s", rt);
    }

    // 6. 44,1 kHz og mix 0 = tørt signal uendret.
    sett (p, "mix", 0.0f);
    auto torr = impulsrespons (p, 44100.0, 1.0);
    sjekk (std::abs (torr.getSample (0, 0) - 1.0f) < 1e-4f && energi (torr, 44100, 0.001, 1.0) < 1e-8,
           "mix 0 = tørt (44,1 kHz)", torr.getSample (0, 0));
    sett (p, "mix", 100.0f);
    sett (p, "lengde_s", 2.4f);
    auto ir44 = impulsrespons (p, 44100.0, 4.0);
    sjekk (std::abs (maalRT (ir44, 44100) - 2.4) < 0.4, "RT ved 44,1 kHz", maalRT (ir44, 44100));

    // 6b. RTA: analysatoren gir riktig nivå og frekvens, og prosessoren sender data.
    {
        AnalyseFifo fifo;
        Spektrum sp;
        std::vector<float> sinus (2048);
        int fase = 0;
        for (int runde = 0; runde < 30; ++runde)
        {
            for (auto& v : sinus) v = 0.5f * std::sin (MathConstants<float>::twoPi * 1000.0f * (float) (fase++) / 48000.0f);
            fifo.push (sinus.data(), sinus.data(), (int) sinus.size());
            sp.hent (fifo);
            sp.beregn (48000.0);
        }
        sjekk (std::abs (sp.db (1000.0) + 6.02) < 1.5, "RTA: 1 kHz sinus 0,5 = -6 dBFS", sp.db (1000.0));
        sjekk (sp.db (200.0) < -60.0f, "RTA: ingenting ved 200 Hz", sp.db (200.0));
        std::vector<float> mye (100000, 0.1f);
        fifo.push (mye.data(), mye.data(), (int) mye.size()); // full kø skal ikke krasje
        sp.hent (fifo);
        sjekk (true, "RTA: full kø tåles");

        float tmp[512];
        while (p.rtaVaat.pull (tmp, 512) > 0) {}
        MidiBuffer midi;
        AudioBuffer<float> b (2, 512);
        b.clear(); b.setSample (0, 0, 1.0f); b.setSample (1, 0, 1.0f);
        p.processBlock (b, midi);
        sjekk (p.rtaVaat.pull (tmp, 512) == 512 && p.rtaInn.pull (tmp, 512) > 0, "prosessoren sender RTA-data");
    }

    // 7. Lagring/gjenoppretting av tilstand.
    sett (p, "predelay_ms", 33.0f);
    MemoryBlock mb;
    p.getStateInformation (mb);
    FiolinProcessor p2;
    p2.setStateInformation (mb.getData(), (int) mb.getSize());
    sjekk (std::abs (p2.parametre.getRawParameterValue ("predelay_ms")->load() - 33.0f) < 0.01f, "tilstand lagres");

    // 8. Grensesnittet, i standardoppsett og ved 1,5x størrelse.
    {
        FiolinProcessor p3;
        std::unique_ptr<AudioProcessorEditor> ed (p3.createEditor());
        sjekk (ed != nullptr && ed->getWidth() > 0, "editor lages", ed->getWidth());
        auto lagre = [&] (const char* navn)
        {
            auto bilde = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
            File fil (utMappe.getChildFile (navn));
            fil.deleteFile();
            FileOutputStream ut (fil);
            PNGImageFormat().writeImageToStream (bilde, ut);
        };
        ed->setSize (FiolinEditor::basisBredde * 3 / 2, FiolinEditor::basisHoyde * 3 / 2);
        lagre ("screenshot-decay.png");
        // EQ-visning med litt EQ på
        sett (p3, "lavkutt", 120.0f); sett (p3, "hishelf_gain", -4.5f); sett (p3, "loshelf_gain", -2.0f);
        sett (p3, "hoykutt", 12000.0f); sett (p3, "diffusion", 40.0f);
        std::function<KlangVisning* (Component*)> finn = [&] (Component* c) -> KlangVisning*
        {
            if (auto* k = dynamic_cast<KlangVisning*> (c)) return k;
            for (auto* barn : c->getChildren()) if (auto* k = finn (barn)) return k;
            return nullptr;
        };
        // Spill et fiolinlignende signal gjennom så RTA-en har noe å vise.
        sett (p3, "mix", 100.0f);
        p3.prepareToPlay (48000.0, 512);
        Thread::sleep (300);
        auto* kv = finn (ed.get());
        if (kv != nullptr) kv->settModus (KlangVisning::Modus::eq);
        {
            MidiBuffer midi;
            Random rng (3);
            int n0 = 0;
            for (int blokk = 0; blokk < 300; ++blokk)
            {
                AudioBuffer<float> b (2, 512);
                for (int i = 0; i < 512; ++i)
                {
                    const float t = (float) (n0 + i) / 48000.0f;
                    const float f = 440.0f * (1.0f + 0.006f * std::sin (6.28f * 5.5f * t));
                    float v = 0;
                    for (int h = 1; h < 20; ++h) v += std::sin (6.2831853f * f * (float) h * t) / (float) h;
                    v = 0.15f * v + 0.003f * (rng.nextFloat() - 0.5f);
                    b.setSample (0, i, v); b.setSample (1, i, v);
                }
                n0 += 512;
                p3.processBlock (b, midi);
                if (kv != nullptr && blokk % 8 == 0) kv->oppdaterNaa();
            }
        }
        // Fanene: vis EQ som aktiv (timeren som ellers gjør dette, går ikke her).
        std::function<void (Component*)> faner = [&] (Component* c)
        {
            if (auto* b = dynamic_cast<TextButton*> (c))
            {
                if (b->getButtonText() == "EQ") b->setToggleState (true, dontSendNotification);
                if (b->getButtonText() == "DECAY") b->setToggleState (false, dontSendNotification);
            }
            for (auto* barn : c->getChildren()) faner (barn);
        };
        faner (ed.get());
        lagre ("screenshot.png");
    }

    std::printf ("%s: %d feil\n", feil == 0 ? "BESTÅTT" : "FEILET", feil);
    return feil == 0 ? 0 : 1;
}
