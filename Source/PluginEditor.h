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

// Display: DECAY (hvordan klangen dør ut), EQ (filterkurve + RTA) eller
// DYN EQ (dynamisk EQ + RTA). Håndtakene i EQ og DYN EQ kan dras med musen.
class KlangVisning : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
{
public:
    enum class Modus { decay, eq, dyn };
    explicit KlangVisning (FiolinProcessor&);
    void paint (juce::Graphics&) override;
    void settModus (Modus m) { if (m != modus) { modus = m; hover = -1; repaint(); } }
    Modus hentModus() const { return modus; }
    void oppdaterNaa() { timerCallback(); } // for tester/skjermbilder

    // Håndtak (offentlig for testene).
    int antallHandtak() const { return (int) handtak().size(); }
    juce::Point<float> handtakPos (int i) const { return handtak()[(size_t) i].pos; }
    juce::String handtakNavn (int i) const { return handtak()[(size_t) i].tekst; }
    int finnHandtak (juce::Point<float>) const;
    void flyttHandtak (int i, juce::Point<float>);

    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    enum class YType { ingen, gainHalv, terskel, range };
    struct Handtak
    {
        juce::Point<float> pos;
        juce::String xId, yId, qId, tekst;
        YType y = YType::ingen;
        bool aktiv = true;
        juce::Colour farge;
    };
    std::vector<Handtak> handtak() const;
    juce::Rectangle<float> plottOmr() const;
    float fx (double f) const;
    double fFraX (float x) const;
    float eqY (float db) const;
    float eqDb (float y) const;
    float rtaY (float db) const;
    float rtaDb (float y) const;
    float v (const juce::String& id) const;
    void settParam (const juce::String& id, float verdi);
    void gest (const Handtak&, bool start);

    void timerCallback() override;
    void tegnDecay (juce::Graphics&, juce::Rectangle<float> plott);
    void tegnEq (juce::Graphics&, juce::Rectangle<float> plott);
    void tegnDyn (juce::Graphics&, juce::Rectangle<float> plott);
    void tegnFrekvensRutenett (juce::Graphics&, juce::Rectangle<float> plott);
    void tegnRta (juce::Graphics&, juce::Rectangle<float> plott, const Spektrum& linje,
                  const Spektrum& fyll, bool tilt);
    void tegnHandtak (juce::Graphics&);
    FiolinProcessor& proc;
    juce::AudioProcessorValueTreeState& p;
    Spektrum spInn, spVaat, spFoerDyn;
    std::vector<float> sist;
    Modus modus = Modus::decay;
    int hover = -1, drar = -1;
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
    std::array<juce::TextButton, 3> faner;
    Knott decay, predelay, diffusion, tidlige,
          lavkutt, loFreq, loGain, hoykutt, hiFreq, hiGain,
          bredde, mix, modDybde, modRate;
    std::array<Knott, FiolinProcessor::antallDyn> dynFreq, dynQ, dynThr, dynRange;
    int visteModus = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FiolinEditor)
};
