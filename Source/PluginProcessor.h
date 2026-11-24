#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include "Analysator.h"

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

    juce::dsp::Convolution konvolusjon { juce::dsp::Convolution::NonUniform { 512 } };
    juce::dsp::DryWetMixer<float> mikser { 8192 };
    juce::dsp::StateVariableTPTFilter<float> lavkutt, hoykutt;
    juce::SmoothedValue<float> breddeSmooth;
    void oppdaterShelf();
    std::array<juce::dsp::IIR::Filter<float>, 2> loShelf, hiShelf;
    std::array<float, 4> shelfSist { -1, -1, -1, -1 };
    juce::AudioBuffer<float> vaat;

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
