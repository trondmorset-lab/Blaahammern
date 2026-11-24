#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "RomSyntese.h"

using namespace juce;

namespace
{
    String des (float v, int n) { return String (v, n); }
    // Godtar både «3.7» og «3,7».
    float lesTall (const String& s) { return s.replaceCharacter (',', '.').retainCharacters ("0123456789.-").getFloatValue(); }
    String hzTekst (float v) { return v >= 1000.0f ? des (v / 1000.0f, 1) + " kHz" : String (roundToInt (v)) + " Hz"; }
    float lesHz (const String& s) { const float v = lesTall (s); return s.containsIgnoreCase ("k") ? v * 1000.0f : v; }

    NormalisableRange<float> logRange (float lo, float hi, float steg)
    {
        NormalisableRange<float> r (lo, hi, steg);
        r.setSkewForCentre (std::sqrt (lo * hi));
        return r;
    }
}

StringArray FiolinProcessor::romNavn()
{
    StringArray s;
    for (const auto& r : romsyntese::rom()) s.add (r.navn);
    return s;
}

AudioProcessorValueTreeState::ParameterLayout FiolinProcessor::lagParametre()
{
    using P = AudioParameterFloat;
    using A = AudioParameterFloatAttributes;
    auto tolk = [] (const String& s) { return lesTall (s); };

    auto prosent = A().withLabel ("%").withValueFromStringFunction (tolk)
                       .withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)) + " %"; });
    auto sek = A().withLabel ("s").withValueFromStringFunction (tolk)
                   .withStringFromValueFunction ([] (float v, int) { return des (v, 1) + " s"; });
    auto ms = A().withLabel ("ms").withValueFromStringFunction (tolk)
                  .withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)) + " ms"; });
    auto db = A().withLabel ("dB").withValueFromStringFunction (tolk)
                  .withStringFromValueFunction ([] (float v, int)
                  {
                      return (v > 0.05f ? "+" : "") + des (std::abs (v) < 0.05f ? 0.0f : v, 1) + " dB";
                  });
    auto hz = A().withLabel ("Hz").withValueFromStringFunction (lesHz)
                  .withStringFromValueFunction ([] (float v, int) { return hzTekst (v); });
    auto hzHoy = A().withLabel ("Hz").withValueFromStringFunction ([] (const String& s)
                  {
                      return s.containsIgnoreCase ("off") ? 20000.0f : lesHz (s);
                  })
                  .withStringFromValueFunction ([] (float v, int) { return v >= 19950.0f ? String ("Off") : hzTekst (v); });
    auto hzLav = A().withLabel ("Hz").withValueFromStringFunction ([] (const String& s)
                  {
                      return s.containsIgnoreCase ("off") ? 20.0f : lesHz (s);
                  })
                  .withStringFromValueFunction ([] (float v, int) { return v <= 20.5f ? String ("Off") : hzTekst (v); });

    NormalisableRange<float> lengdeRange (0.3f, 10.0f, 0.1f);
    lengdeRange.setSkewForCentre (2.5f);
    NormalisableRange<float> preRange (0.0f, 150.0f, 1.0f);
    preRange.setSkewForCentre (35.0f);
    NormalisableRange<float> lavRange (20.0f, 500.0f, 1.0f);
    lavRange.setSkewForCentre (120.0f);
    NormalisableRange<float> hoyRange (2000.0f, 20000.0f, 10.0f);
    hoyRange.setSkewForCentre (8000.0f);

    // Parameter-ID-ene er de samme som før, så lagrede prosjekter virker.
    return {
        std::make_unique<AudioParameterChoice> (ParameterID { "rom", 1 }, "Room", romNavn(), 0),
        std::make_unique<P> (ParameterID { "lengde_s", 2 }, "Decay", lengdeRange, 2.4f, sek),
        std::make_unique<P> (ParameterID { "predelay_ms", 2 }, "Pre-Delay", preRange, 25.0f, ms),
        std::make_unique<P> (ParameterID { "tidlige", 2 }, "Early Reflections", NormalisableRange<float> (-24.0f, 6.0f, 0.5f), 0.0f, db),
        std::make_unique<P> (ParameterID { "diffusion", 3 }, "Diffusion", NormalisableRange<float> (0.0f, 100.0f, 1.0f), 80.0f, prosent),
        std::make_unique<P> (ParameterID { "lavkutt", 1 }, "Low Cut", lavRange, 20.0f, hzLav),
        std::make_unique<P> (ParameterID { "loshelf_freq", 3 }, "Low Shelf Freq", logRange (50.0f, 1000.0f, 1.0f), 250.0f, hz),
        std::make_unique<P> (ParameterID { "loshelf_gain", 3 }, "Low Shelf Gain", NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f, db),
        std::make_unique<P> (ParameterID { "hishelf_freq", 3 }, "High Shelf Freq", logRange (1000.0f, 16000.0f, 10.0f), 6000.0f, hz),
        std::make_unique<P> (ParameterID { "hishelf_gain", 3 }, "High Shelf Gain", NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f, db),
        std::make_unique<P> (ParameterID { "hoykutt", 1 }, "High Cut", hoyRange, 20000.0f, hzHoy),
        std::make_unique<P> (ParameterID { "bredde", 1 }, "Width", NormalisableRange<float> (0.0f, 150.0f, 1.0f), 100.0f, prosent),
        std::make_unique<P> (ParameterID { "mix", 1 }, "Mix", NormalisableRange<float> (0.0f, 100.0f, 0.5f), 25.0f, prosent),
    };
}

namespace
{
    // Shelving-koeffisienter (RBJ, Q = 0,707), brukt både i lyd og visning.
    dsp::IIR::Coefficients<float>::Ptr lagShelf (bool lav, double fs, float f, float gainDb)
    {
        const float g = Decibels::decibelsToGain (gainDb);
        f = jmin (f, (float) fs * 0.45f);
        return lav ? dsp::IIR::Coefficients<float>::makeLowShelf (fs, f, 0.7071f, g)
                   : dsp::IIR::Coefficients<float>::makeHighShelf (fs, f, 0.7071f, g);
    }
}

float FiolinProcessor::eqRespons (const AudioProcessorValueTreeState& ps, double f, double fs)
{
    auto v = [&] (const char* id) { return ps.getRawParameterValue (id)->load(); };
    double db = 0.0;
    db += Decibels::gainToDecibels (lagShelf (true, fs, v ("loshelf_freq"), v ("loshelf_gain"))->getMagnitudeForFrequency (f, fs));
    db += Decibels::gainToDecibels (lagShelf (false, fs, v ("hishelf_freq"), v ("hishelf_gain"))->getMagnitudeForFrequency (f, fs));
    // 2. ordens Butterworth (samme som StateVariableTPTFilter med Q = 0,707).
    const double lc = v ("lavkutt"), hc = v ("hoykutt");
    if (lc > 20.5)
        db += 20.0 * std::log10 ((f * f) / std::sqrt (std::pow (lc * lc - f * f, 2.0) + std::pow (f * lc / 0.7071, 2.0)));
    if (hc < 19950.0)
        db += 20.0 * std::log10 ((hc * hc) / std::sqrt (std::pow (hc * hc - f * f, 2.0) + std::pow (f * hc / 0.7071, 2.0)));
    return (float) db;
}

FiolinProcessor::FiolinProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", AudioChannelSet::stereo(), true)
                          .withOutput ("Output", AudioChannelSet::stereo(), true)),
      parametre (*this, nullptr, "Fiolinklang", lagParametre())
{
    pRom      = parametre.getRawParameterValue ("rom");
    pMix      = parametre.getRawParameterValue ("mix");
    pPredelay = parametre.getRawParameterValue ("predelay_ms");
    pLengde   = parametre.getRawParameterValue ("lengde_s");
    pTidlige  = parametre.getRawParameterValue ("tidlige");
    pLavkutt  = parametre.getRawParameterValue ("lavkutt");
    pHoykutt  = parametre.getRawParameterValue ("hoykutt");
    pBredde   = parametre.getRawParameterValue ("bredde");
    pDiffusion = parametre.getRawParameterValue ("diffusion");
    pLoFreq   = parametre.getRawParameterValue ("loshelf_freq");
    pLoGain   = parametre.getRawParameterValue ("loshelf_gain");
    pHiFreq   = parametre.getRawParameterValue ("hishelf_freq");
    pHiGain   = parametre.getRawParameterValue ("hishelf_gain");

    mikser.setMixingRule (dsp::DryWetMixingRule::balanced);
    lavkutt.setType (dsp::StateVariableTPTFilterType::highpass);
    hoykutt.setType (dsp::StateVariableTPTFilterType::lowpass);

    traad.startThread (Thread::Priority::low);
    startTimerHz (20);
}

FiolinProcessor::~FiolinProcessor()
{
    stopTimer();
    traad.signalThreadShouldExit();
    traad.notify();
    traad.stopThread (4000);
}

FiolinProcessor::Oppsett FiolinProcessor::lesOppsett() const
{
    Oppsett o;
    o.rom = jlimit (0, 2, (int) pRom->load());
    o.rt = jlimit (0.3f, 10.0f, pLengde->load());
    o.pre = jlimit (0.0f, 150.0f, pPredelay->load());
    o.tidlige = pTidlige->load();
    o.diff = pDiffusion->load();
    o.fs = rate.load();
    return o;
}

void FiolinProcessor::oppdaterIR()
{
    const auto o = lesOppsett();
    if (! (o == bedt))
    {
        bedt = o;
        traad.notify();
    }
}

void FiolinProcessor::byggIR (bool tving)
{
    const ScopedLock sl (irLaas);
    const auto o = lesOppsett();
    if (! tving && o == lastet)
        return;
    lastet = o;

    auto ir = romsyntese::lagIR (romsyntese::rom()[(size_t) o.rom], o.rt, o.pre * 0.001,
                                 o.tidlige, o.diff * 0.01, o.fs);
    konvolusjon.loadImpulseResponse (std::move (ir), o.fs,
                                     dsp::Convolution::Stereo::yes,
                                     dsp::Convolution::Trim::no,
                                     dsp::Convolution::Normalise::no);
}

void FiolinProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    rate = sampleRate;
    const dsp::ProcessSpec spec { sampleRate, (uint32) samplesPerBlock, 2 };

    {
        const ScopedLock sl (irLaas);
        konvolusjon.prepare (spec);
    }
    byggIR (true);
    bedt = lastet;

    mikser.prepare (spec);
    mikser.setWetLatency ((float) konvolusjon.getLatency());
    setLatencySamples (0);

    lavkutt.prepare (spec);
    hoykutt.prepare (spec);
    lavkutt.setResonance (0.7071f);
    hoykutt.setResonance (0.7071f);

    for (auto* f : { &loShelf[0], &loShelf[1], &hiShelf[0], &hiShelf[1] })
    {
        f->prepare ({ sampleRate, (uint32) samplesPerBlock, 1 });
        f->reset();
    }
    shelfSist = { -1, -1, -1, -1 };
    oppdaterShelf();

    breddeSmooth.reset (sampleRate, 0.05);
    breddeSmooth.setCurrentAndTargetValue (pBredde->load() / 100.0f);

    vaat.setSize (2, samplesPerBlock);
}

void FiolinProcessor::oppdaterShelf()
{
    const std::array<float, 4> na { pLoFreq->load(), pLoGain->load(), pHiFreq->load(), pHiGain->load() };
    if (na == shelfSist)
        return;
    shelfSist = na;
    // Koeffisientene byttes på lydtråden; et lite oppslag er godt nok her
    // (endres bare når knappen vris).
    auto lo = lagShelf (true, rate.load(), na[0], na[1]);
    auto hi = lagShelf (false, rate.load(), na[2], na[3]);
    for (int ch = 0; ch < 2; ++ch)
    {
        *loShelf[(size_t) ch].coefficients = *lo;
        *hiShelf[(size_t) ch].coefficients = *hi;
    }
}

bool FiolinProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto inn = layouts.getMainInputChannelSet();
    const auto ut = layouts.getMainOutputChannelSet();
    return ut == AudioChannelSet::stereo()
        && (inn == AudioChannelSet::stereo() || inn == AudioChannelSet::mono());
}

void FiolinProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2 || n == 0)
        return;

    if (getTotalNumInputChannels() == 1)
        buffer.copyFrom (1, 0, buffer, 0, 0, n);

    if (vaat.getNumSamples() < n)
        vaat.setSize (2, n, false, false, true);

    auto blokk = dsp::AudioBlock<float> (buffer).getSubsetChannelBlock (0, 2);
    mikser.setWetMixProportion (pMix->load() / 100.0f);
    mikser.pushDrySamples (blokk);

    // Våt gren: konvolusjon (predelay ligger i IR-en) -> low/high cut -> bredde.
    vaat.copyFrom (0, 0, buffer, 0, 0, n);
    vaat.copyFrom (1, 0, buffer, 1, 0, n);
    auto vaatBlokk = dsp::AudioBlock<float> (vaat).getSubsetChannelBlock (0, 2).getSubBlock (0, (size_t) n);
    dsp::ProcessContextReplacing<float> ctx (vaatBlokk);
    konvolusjon.process (ctx);

    const double fs = rate.load();
    lavkutt.setCutoffFrequency (jmin (pLavkutt->load(), (float) fs * 0.45f));
    hoykutt.setCutoffFrequency (jmin (pHoykutt->load(), (float) fs * 0.45f));
    if (pLavkutt->load() > 20.5f)  lavkutt.process (ctx);
    if (pHoykutt->load() < 19950.0f) hoykutt.process (ctx);

    oppdaterShelf();
    for (int ch = 0; ch < 2; ++ch)
    {
        auto kanal = vaatBlokk.getSingleChannelBlock ((size_t) ch);
        dsp::ProcessContextReplacing<float> c1 (kanal);
        if (std::abs (shelfSist[1]) > 0.05f) loShelf[(size_t) ch].process (c1);
        if (std::abs (shelfSist[3]) > 0.05f) hiShelf[(size_t) ch].process (c1);
    }

    breddeSmooth.setTargetValue (pBredde->load() / 100.0f);
    float* l = vaat.getWritePointer (0);
    float* r = vaat.getWritePointer (1);
    for (int i = 0; i < n; ++i)
    {
        const float w = breddeSmooth.getNextValue();
        const float m = 0.5f * (l[i] + r[i]);
        const float s = 0.5f * (l[i] - r[i]) * w;
        l[i] = m + s;
        r[i] = m - s;
    }

    rtaInn.push (buffer.getReadPointer (0), buffer.getReadPointer (1), n);
    rtaVaat.push (vaat.getReadPointer (0), vaat.getReadPointer (1), n);

    buffer.copyFrom (0, 0, vaat, 0, 0, n);
    buffer.copyFrom (1, 0, vaat, 1, 0, n);
    mikser.mixWetSamples (blokk);
}

void FiolinProcessor::getStateInformation (MemoryBlock& destData)
{
    if (auto xml = parametre.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void FiolinProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (parametre.state.getType()))
            parametre.replaceState (ValueTree::fromXml (*xml));
    oppdaterIR();
}

AudioProcessorEditor* FiolinProcessor::createEditor()
{
    return new FiolinEditor (*this);
}

#if ! T1REVPRO_TEST
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FiolinProcessor();
}
#endif
