#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include "Analysator.h"
#include "Modulasjon.h"

// T1 revPRO: konvolusjonsklang der impulsresponsen syntetiseres i pluginen
// (tre romtyper, fritt valgt lengde i sekunder).
class FiolinProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    FiolinProcessor();
    ~FiolinProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "T1 revPRO"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 13.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Ber bakgrunnstråden lage ny IR hvis rom/decay/predelay/ER/diffusion er endret.
    void oppdaterIR();
    // Lager og laster IR med en gang (prepareToPlay, tester).
    void byggIR (bool tving);

    juce::AudioProcessorValueTreeState parametre;
    static juce::StringArray romNavn();

    // RTA: inngang (før klang) og klang etter EQ, til visningen.
    AnalyseFifo rtaInn, rtaVaat;
    double hentRate() const { return rate.load(); }

    // Samlet EQ-respons (low cut, low shelf, high shelf, high cut) i dB ved
    // frekvens f, for EQ-visningen.
    static float eqRespons (const juce::AudioProcessorValueTreeState&, double f, double fs = 48000.0);

    // Mix-kurven: {tørr gain, våt gain} for Mix i prosent.
    static std::pair<float, float> mixGain (float mixProsent);

    // Dynamisk EQ: to båndfiltre på klangen som demper bare når nivået i
    // båndet går over terskelen (Thresh), med maks demping Range.
    static constexpr int antallDyn = 2;
    static juce::String dynId (int band, const char* navn) { return "dyn" + juce::String (band + 1) + "_" + navn; }
    // Respons i dB ved f for en peaking-EQ (brukes av visningen).
    static float toppRespons (double f, double f0, double q, double gainDb, double fs = 48000.0);
    // Nåværende demping per bånd (dB, <= 0), for visningen.
    float hentDynGr (int band) const { return dynGr[(size_t) band].load(); }
    // Nivå i båndet slik detektoren ser det (dBFS), for visningen.
    float hentDynNivaa (int band) const { return dynNivaa[(size_t) band].load(); }
    // RTA av klangen før den dynamiske EQ-en (DYN EQ-visningen).
    AnalyseFifo rtaFoerDyn;

private:
public:
    // Der IR-en deles: så lenge etter predelay starter halen (s), og
    // overgangen (s).
    static constexpr double delingEtterPre = 0.035, delingOvergang = 0.02;

private:
    struct Oppsett
    {
        int rom = -1; float rt = 0, pre = 0, tidlige = 0, diff = 0; double fs = 0;
        bool operator== (const Oppsett& o) const
        {
            return rom == o.rom && rt == o.rt && pre == o.pre && tidlige == o.tidlige
                && diff == o.diff && fs == o.fs;
        }
    };
    Oppsett lesOppsett() const;

    class IRTraad : public juce::Thread
    {
    public:
        explicit IRTraad (FiolinProcessor& p) : Thread ("T1 revPRO IR"), eier (p) {}
        void run() override
        {
            while (! threadShouldExit())
            {
                wait (-1);
                if (! threadShouldExit())
                    eier.byggIR (false);
            }
        }
    private:
        FiolinProcessor& eier;
    };

    void timerCallback() override { oppdaterIR(); }
    static juce::AudioProcessorValueTreeState::ParameterLayout lagParametre();

    juce::CriticalSection irLaas;
    Oppsett lastet, bedt;
    std::atomic<double> rate { 48000.0 };

    // IR-en deles i to: starten (tidlige refleksjoner, umodulert) og halen,
    // som går gjennom modulasjonen.
    juce::dsp::Convolution konvTidlig { juce::dsp::Convolution::NonUniform { 512 } };
    juce::dsp::Convolution konvHale { juce::dsp::Convolution::NonUniform { 512 } };
    Modulasjon modulasjon;
    juce::AudioBuffer<float> haleBuf;
    std::atomic<float>* pModDybde = nullptr;
    std::atomic<float>* pModRate = nullptr;
    juce::SmoothedValue<float> torrGain, vaatGain;
    juce::dsp::StateVariableTPTFilter<float> lavkutt, hoykutt;
    juce::SmoothedValue<float> breddeSmooth;
    void oppdaterShelf();
    std::array<juce::dsp::IIR::Filter<float>, 2> loShelf, hiShelf;
    std::array<float, 4> shelfSist { -1, -1, -1, -1 };
    juce::AudioBuffer<float> vaat;

    struct DynBand
    {
        std::atomic<float>* frekv = nullptr;
        std::atomic<float>* q = nullptr;
        std::atomic<float>* terskel = nullptr;
        std::atomic<float>* range = nullptr;
        juce::dsp::StateVariableTPTFilter<float> detektor;
        float konvolutt = 0.0f;      // effekt (kvadrat) i båndet
        float gr = 0.0f;             // demping nå (dB)
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
        std::array<double, 2> z1 {}, z2 {};
    };
    std::array<DynBand, antallDyn> dyn;
    std::array<std::atomic<float>, antallDyn> dynGr {}, dynNivaa {};
    void prosesserDyn (float* l, float* r, int n);

    std::atomic<float>* pRom = nullptr;
    std::atomic<float>* pMix = nullptr;
    std::atomic<float>* pPredelay = nullptr;
    std::atomic<float>* pLengde = nullptr;
    std::atomic<float>* pTidlige = nullptr;
    std::atomic<float>* pLavkutt = nullptr;
    std::atomic<float>* pHoykutt = nullptr;
    std::atomic<float>* pBredde = nullptr;
    std::atomic<float>* pDiffusion = nullptr;
    std::atomic<float>* pLoFreq = nullptr;
    std::atomic<float>* pLoGain = nullptr;
    std::atomic<float>* pHiFreq = nullptr;
    std::atomic<float>* pHiGain = nullptr;

    IRTraad traad { *this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FiolinProcessor)
};
