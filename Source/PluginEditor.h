#pragma once

#include "PluginProcessor.h"
#include <juce_audio_utils/juce_audio_utils.h>

class FiolinLook : public juce::LookAndFeel_V4
{
public:
    FiolinLook();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float start, float end, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&,
                               bool over, bool down) override;
    juce::Font getTextButtonFont (juce::TextButton&, int h) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
};

// Display: DECAY (hvordan klangen dør ut) eller EQ (filterkurve på klangen).
class KlangVisning : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
{
public:
    enum class Modus { decay, eq };
    explicit KlangVisning (FiolinProcessor&);
    void paint (juce::Graphics&) override;
    void settModus (Modus m) { if (m != modus) { modus = m; repaint(); } }
    Modus hentModus() const { return modus; }
    void oppdaterNaa() { timerCallback(); } // for tester/skjermbilder

private:
    void timerCallback() override;
    void tegnDecay (juce::Graphics&, juce::Rectangle<float> plott);
    void tegnEq (juce::Graphics&, juce::Rectangle<float> plott);
    void tegnRta (juce::Graphics&, juce::Rectangle<float> plott);
    FiolinProcessor& proc;
    juce::AudioProcessorValueTreeState& p;
    Spektrum spInn, spVaat;
    std::vector<float> sist;
    Modus modus = Modus::decay;
};

class FiolinEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit FiolinEditor (FiolinProcessor&);
    ~FiolinEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int basisBredde = 992, basisHoyde = 570;

private:
    struct Knott
    {
        juce::Slider slider;
        juce::Label navn;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> kobling;
    };

    class Innhold : public juce::Component
    {
    public:
        explicit Innhold (FiolinEditor& e) : ed (e) {}
        void paint (juce::Graphics&) override;
        void resized() override;
    private:
        FiolinEditor& ed;
    };

    void timerCallback() override;
    void lagKnott (Knott&, const juce::String& id, const juce::String& navn, const juce::String& hjelp,
                   KlangVisning::Modus visning);
    void velgRom (int rom);
    void velgVisning (KlangVisning::Modus m);
    void plasser (Knott&, juce::Rectangle<int> celle, int diameter);

    FiolinProcessor& prosessor;
    FiolinLook utseende;
    Innhold innhold { *this };
    juce::TooltipWindow hjelpetekst { this, 600 };
    KlangVisning visning;
    std::array<juce::TextButton, 3> romKnapper;
    std::array<juce::TextButton, 2> faner;
    Knott decay, predelay, diffusion, tidlige,
          lavkutt, loFreq, loGain, hoykutt, hiFreq, hiGain,
          bredde, mix;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FiolinEditor)
};
