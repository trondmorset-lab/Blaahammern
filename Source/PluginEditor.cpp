#include "PluginEditor.h"
#include "RomSyntese.h"
#include "BinaryData.h"

using namespace juce;

namespace farge
{
    const Colour bakgrunn { 0xff0a101c };
    const Colour panel    { 0xff111a2a };
    const Colour panel2   { 0xff18243a };
    const Colour linje    { 0xff26354f };
    const Colour aksent   { 0xff4ea8ff }; // klar blå
    const Colour aksent2  { 0xff1f5fd0 }; // dyp blå
    const Colour lys      { 0xffeef2f7 }; // sølv/hvit
    const Colour tekst    { 0xffd7dfea };
    const Colour dempet   { 0xff8392a8 };
    const Colour diskant  { 0xffc9d3df }; // sølv
    const Colour bass     { 0xff8c7dff }; // blåfiolett
    const Colour solvLys  { 0xffe4e9ef };
    const Colour solvMork { 0xff8994a3 };
}

//==============================================================================
FiolinLook::FiolinLook()
{
    setColour (Slider::textBoxTextColourId, farge::tekst);
    setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    setColour (Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    setColour (Slider::textBoxHighlightColourId, farge::aksent.withAlpha (0.4f));
    setColour (Label::textColourId, farge::dempet);
    setColour (Label::textWhenEditingColourId, farge::lys);
    setColour (Label::outlineWhenEditingColourId, farge::aksent);
    setColour (TextEditor::backgroundColourId, farge::panel2);
    setColour (TextEditor::textColourId, farge::lys);
    setColour (TextEditor::highlightColourId, farge::aksent.withAlpha (0.4f));
    setColour (CaretComponent::caretColourId, farge::aksent);
    setColour (TextButton::textColourOffId, farge::dempet);
    setColour (TextButton::textColourOnId, Colours::white);
    setColour (TooltipWindow::backgroundColourId, farge::panel2);
    setColour (TooltipWindow::textColourId, farge::tekst);
    setColour (TooltipWindow::outlineColourId, farge::linje);
}

void FiolinLook::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                                   float start, float end, Slider& s)
{
    const auto omr = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
    const float r = jmin (omr.getWidth(), omr.getHeight()) * 0.5f;
    const auto c = omr.getCentre();
    const float tykk = jmax (2.5f, r * 0.075f);
    const float rb = r - tykk * 1.6f;
    const float vinkel = start + pos * (end - start);
    const bool over = s.isMouseOverOrDragging();

    // spor
    Path spor;
    spor.addCentredArc (c.x, c.y, rb, rb, 0.0f, start, end, true);
    g.setColour (farge::linje);
    g.strokePath (spor, PathStrokeType (tykk, PathStrokeType::curved, PathStrokeType::rounded));

    // verdi med glød
    Path verdi;
    verdi.addCentredArc (c.x, c.y, rb, rb, 0.0f, start, vinkel, true);
    g.setColour (farge::aksent.withAlpha (over ? 0.28f : 0.16f));
    g.strokePath (verdi, PathStrokeType (tykk * 3.0f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setGradientFill (ColourGradient (farge::aksent2, c.x - r, c.y + r,
                                       over ? farge::lys : farge::aksent, c.x + r, c.y - r, false));
    g.strokePath (verdi, PathStrokeType (tykk, PathStrokeType::curved, PathStrokeType::rounded));

    // børstet sølvknott
    const float rk = rb - tykk * 2.2f;
    const auto kropp = Rectangle<float> (rk * 2, rk * 2).withCentre (c);
    g.setColour (Colours::black.withAlpha (0.45f));
    g.fillEllipse (kropp.translated (0, rk * 0.10f).expanded (rk * 0.04f));
    g.setGradientFill (ColourGradient (farge::solvLys, c.x, c.y - rk, farge::solvMork, c.x, c.y + rk, false));
    g.fillEllipse (kropp);
    const auto flate = kropp.reduced (rk * 0.12f);
    g.setGradientFill (ColourGradient (farge::solvMork.brighter (0.25f), c.x, c.y - rk,
                                       farge::solvLys.darker (0.05f), c.x, c.y + rk, false));
    g.fillEllipse (flate);
    g.setColour (Colours::black.withAlpha (0.35f));
    g.drawEllipse (kropp, 1.0f);

    // viser
    const Point<float> p1 = c.getPointOnCircumference (rk * 0.30f, vinkel);
    const Point<float> p2 = c.getPointOnCircumference (rk * 0.78f, vinkel);
    g.setColour (farge::bakgrunn);
    g.drawLine ({ p1, p2 }, jmax (2.0f, r * 0.055f));
    g.setColour (over ? farge::aksent.brighter (0.3f) : farge::aksent);
    g.fillEllipse (Rectangle<float> (r * 0.09f, r * 0.09f).withCentre (c.getPointOnCircumference (rk * 0.80f, vinkel)));
}

void FiolinLook::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool over, bool down)
{
    auto omr = b.getLocalBounds().toFloat().reduced (1.0f);
    const bool paa = b.getToggleState();
    const auto kanter = b.isConnectedOnLeft() || b.isConnectedOnRight();
    Path p;
    p.addRoundedRectangle (omr.getX(), omr.getY(), omr.getWidth(), omr.getHeight(), 6.0f, 6.0f,
                           ! b.isConnectedOnLeft(), ! b.isConnectedOnRight(),
                           ! b.isConnectedOnLeft(), ! b.isConnectedOnRight());
    ignoreUnused (kanter);
    if (paa)
        g.setGradientFill (ColourGradient (farge::aksent.brighter (0.1f), 0, omr.getY(),
                                           farge::aksent.darker (0.15f), 0, omr.getBottom(), false));
    else
        g.setColour (over ? farge::panel2.brighter (0.1f) : farge::panel2);
    g.fillPath (p);
    g.setColour (paa ? farge::aksent : farge::linje.brighter (down ? 0.4f : 0.1f));
    g.strokePath (p, PathStrokeType (1.0f));
}

Font FiolinLook::getTextButtonFont (TextButton&, int h)
{
    return FontOptions ((float) h * 0.46f, Font::bold);
}

Label* FiolinLook::createSliderTextBox (Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (FontOptions (13.0f, Font::bold));
    l->setColour (Label::textColourId, farge::tekst);
    l->setColour (Label::backgroundColourId, Colours::transparentBlack);
    l->setColour (Label::outlineColourId, Colours::transparentBlack);
    return l;
}

//==============================================================================
namespace
{
    String tall (double v, int des) { return String (v, des); }
    const String prikk = String (CharPointer_UTF8 ("  \xc2\xb7  "));
}

KlangVisning::KlangVisning (FiolinProcessor& pr) : proc (pr), p (pr.parametre)
{
    startTimerHz (30);
}

void KlangVisning::timerCallback()
{
    std::vector<float> na;
    for (auto* par : p.processor.getParameters())
        na.push_back (par->getValue());
    // Tøm RTA-køene hele tiden, så visningen er fersk når fanen åpnes.
    spInn.hent (proc.rtaInn);
    spVaat.hent (proc.rtaVaat);
    spFoerDyn.hent (proc.rtaFoerDyn);
    if (modus != Modus::decay)
    {
        spVaat.beregn (proc.hentRate());
        (modus == Modus::eq ? spInn : spFoerDyn).beregn (proc.hentRate());
        sist = std::move (na);
        repaint();
        return;
    }
    if (na != sist) { sist = std::move (na); repaint(); }
}

Rectangle<float> KlangVisning::plottOmr() const
{
    return getLocalBounds().toFloat().withTrimmedLeft (50).withTrimmedRight (modus == Modus::decay ? 18 : 46)
               .withTrimmedTop (42).withTrimmedBottom (26);
}

namespace
{
    constexpr double fMin = 20.0, fMaks = 20000.0;
    constexpr float dbMaks = 18.0f, rtaMin = -90.0f, rtaMaks = 0.0f;
}

float KlangVisning::fx (double f) const
{
    const auto r = plottOmr();
    return r.getX() + (float) (std::log (f / fMin) / std::log (fMaks / fMin)) * r.getWidth();
}

double KlangVisning::fFraX (float x) const
{
    const auto r = plottOmr();
    return fMin * std::pow (fMaks / fMin, (double) ((x - r.getX()) / r.getWidth()));
}

float KlangVisning::eqY (float db) const
{
    const auto r = plottOmr();
    return r.getCentreY() - jlimit (-dbMaks, dbMaks, db) / dbMaks * r.getHeight() * 0.5f;
}

float KlangVisning::eqDb (float y) const
{
    const auto r = plottOmr();
    return (r.getCentreY() - y) / (r.getHeight() * 0.5f) * dbMaks;
}

float KlangVisning::rtaY (float db) const
{
    const auto r = plottOmr();
    return r.getY() + (rtaMaks - jlimit (rtaMin, rtaMaks, db)) / (rtaMaks - rtaMin) * r.getHeight();
}

float KlangVisning::rtaDb (float y) const
{
    const auto r = plottOmr();
    return rtaMaks - (y - r.getY()) / r.getHeight() * (rtaMaks - rtaMin);
}

float KlangVisning::v (const String& id) const { return p.getRawParameterValue (id)->load(); }

void KlangVisning::settParam (const String& id, float verdi)
{
    if (auto* par = p.getParameter (id))
    {
        const auto& r = p.getParameterRange (id);
        par->setValueNotifyingHost (r.convertTo0to1 (r.snapToLegalValue (jlimit (r.start, r.end, verdi))));
    }
}

std::vector<KlangVisning::Handtak> KlangVisning::handtak() const
{
    std::vector<Handtak> h;
    if (modus == Modus::eq)
    {
        const float lc = v ("lavkutt"), hc = v ("hoykutt");
        h.push_back ({ { fx (jmax (20.0f, lc)), eqY (FiolinProcessor::eqRespons (p, lc)) }, "lavkutt", {}, {}, "LC", YType::ingen, lc > 20.5f, farge::aksent });
        h.push_back ({ { fx (v ("loshelf_freq")), eqY (v ("loshelf_gain") * 0.5f) }, "loshelf_freq", "loshelf_gain", {}, "LS",
                       YType::gainHalv, std::abs (v ("loshelf_gain")) > 0.05f, farge::aksent });
        h.push_back ({ { fx (v ("hishelf_freq")), eqY (v ("hishelf_gain") * 0.5f) }, "hishelf_freq", "hishelf_gain", {}, "HS",
                       YType::gainHalv, std::abs (v ("hishelf_gain")) > 0.05f, farge::aksent });
        const float hcv = jmin (hc, 19000.0f);
        h.push_back ({ { fx (hcv), eqY (FiolinProcessor::eqRespons (p, hcv)) }, "hoykutt", {}, {}, "HC", YType::ingen, hc < 19950.0f, farge::aksent });
    }
    else if (modus == Modus::dyn)
    {
        for (int b = 0; b < FiolinProcessor::antallDyn; ++b)
        {
            const auto fId = FiolinProcessor::dynId (b, "freq"), qId = FiolinProcessor::dynId (b, "q");
            const float f = v (fId), range = v (FiolinProcessor::dynId (b, "range"));
            const auto c = b == 0 ? farge::aksent : farge::bass;
            h.push_back ({ { fx (f), rtaY (v (FiolinProcessor::dynId (b, "thresh"))) }, fId, FiolinProcessor::dynId (b, "thresh"), qId,
                           String (b + 1), YType::terskel, range > 0.05f, c });
            h.push_back ({ { fx (f), eqY (-range) }, fId, FiolinProcessor::dynId (b, "range"), qId,
                           {}, YType::range, range > 0.05f, c });
        }
    }
    return h;
}

int KlangVisning::finnHandtak (Point<float> pos) const
{
    const auto h = handtak();
    int best = -1;
    float bestAvstand = 14.0f;
    for (size_t i = 0; i < h.size(); ++i)
    {
        const float d = h[i].pos.getDistanceFrom (pos);
        if (d < bestAvstand) { bestAvstand = d; best = (int) i; }
    }
    return best;
}

void KlangVisning::flyttHandtak (int i, Point<float> pos)
{
    const auto h = handtak();
    if (! isPositiveAndBelow (i, (int) h.size()))
        return;
    const auto& ht = h[(size_t) i];
    const auto r = plottOmr();
    settParam (ht.xId, (float) fFraX (jlimit (r.getX(), r.getRight(), pos.x)));
    switch (ht.y)
    {
        case YType::gainHalv: settParam (ht.yId, 2.0f * eqDb (pos.y)); break;
        case YType::terskel:  settParam (ht.yId, rtaDb (pos.y)); break;
        case YType::range:    settParam (ht.yId, -eqDb (pos.y)); break;
        case YType::ingen:    break;
    }
    repaint();
}

void KlangVisning::gest (const Handtak& h, bool start)
{
    for (const auto& id : { h.xId, h.yId })
        if (id.isNotEmpty())
            if (auto* par = p.getParameter (id))
                start ? par->beginChangeGesture() : par->endChangeGesture();
}

void KlangVisning::mouseMove (const MouseEvent& e)
{
    const int na = finnHandtak (e.position);
    if (na != hover)
    {
        hover = na;
        setMouseCursor (na >= 0 ? MouseCursor::DraggingHandCursor : MouseCursor::NormalCursor);
        repaint();
    }
}

void KlangVisning::mouseExit (const MouseEvent&)
{
    if (hover >= 0 && drar < 0) { hover = -1; repaint(); }
}

void KlangVisning::mouseDown (const MouseEvent& e)
{
    drar = finnHandtak (e.position);
    if (drar >= 0)
        gest (handtak()[(size_t) drar], true);
}

void KlangVisning::mouseDrag (const MouseEvent& e)
{
    if (drar >= 0)
        flyttHandtak (drar, e.position);
}

void KlangVisning::mouseUp (const MouseEvent&)
{
    if (drar >= 0)
    {
        const auto h = handtak();
        if (isPositiveAndBelow (drar, (int) h.size()))
            gest (h[(size_t) drar], false);
    }
    drar = -1;
}

void KlangVisning::mouseDoubleClick (const MouseEvent& e)
{
    // Dobbeltklikk på et håndtak: tilbake til standardverdiene.
    const int i = finnHandtak (e.position);
    if (i < 0) return;
    const auto h = handtak()[(size_t) i];
    for (const auto& id : { h.xId, h.yId })
        if (id.isNotEmpty())
            if (auto* par = p.getParameter (id))
            {
                par->beginChangeGesture();
                par->setValueNotifyingHost (par->getDefaultValue());
                par->endChangeGesture();
            }
    repaint();
}

void KlangVisning::mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w)
{
    // Musehjul over et DYN-håndtak endrer Q (båndbredden).
    const int i = finnHandtak (e.position);
    if (i < 0) { Component::mouseWheelMove (e, w); return; }
    const auto h = handtak()[(size_t) i];
    if (h.qId.isEmpty()) return;
    if (auto* par = p.getParameter (h.qId))
    {
        par->beginChangeGesture();
        settParam (h.qId, v (h.qId) * std::pow (1.15f, (w.deltaY > 0 ? 1.0f : -1.0f) * (w.isReversed ? -1.0f : 1.0f)));
        par->endChangeGesture();
    }
    repaint();
}

void KlangVisning::paint (Graphics& g)
{
    const auto alt = getLocalBounds().toFloat();
    g.setColour (farge::panel);
    g.fillRoundedRectangle (alt, 10.0f);
    g.setColour (farge::linje);
    g.drawRoundedRectangle (alt.reduced (0.5f), 10.0f, 1.0f);

    const auto plott = plottOmr();
    if (modus == Modus::decay)   tegnDecay (g, plott);
    else if (modus == Modus::eq) tegnEq (g, plott);
    else                         tegnDyn (g, plott);
}

void KlangVisning::tegnDecay (Graphics& g, Rectangle<float> plott)
{
    const auto alt = getLocalBounds().toFloat();
    const int romNr = jlimit (0, 2, (int) p.getRawParameterValue ("rom")->load());
    const auto& rom = romsyntese::rom()[(size_t) romNr];
    const double rt = p.getRawParameterValue ("lengde_s")->load();
    const double pre = p.getRawParameterValue ("predelay_ms")->load() * 0.001;
    const float tidligeDb = p.getRawParameterValue ("tidlige")->load();
    const float diff = p.getRawParameterValue ("diffusion")->load() * 0.01f;

    const double tMaks = jmax (1.0, std::ceil ((pre + rt * 1.12) * 2.0) / 2.0);
    const float dbMin = -66.0f;
    auto tx = [&] (double t) { return plott.getX() + (float) (t / tMaks) * plott.getWidth(); };
    auto dy = [&] (float db) { return plott.getY() + (jlimit (dbMin, 0.0f, db) / dbMin) * plott.getHeight(); };

    g.setFont (FontOptions (11.0f));
    const double steg = tMaks > 8.0 ? 2.0 : (tMaks > 4.0 ? 1.0 : 0.5);
    for (double t = 0; t <= tMaks + 1e-6; t += steg)
    {
        g.setColour (farge::linje.withAlpha (0.6f));
        g.drawVerticalLine (roundToInt (tx (t)), plott.getY(), plott.getBottom());
        g.setColour (farge::dempet);
        g.drawText (tall (t, t == std::floor (t) ? 0 : 1) + " s",
                    Rectangle<float> (tx (t) - 24, plott.getBottom() + 4, 48, 16), Justification::centred);
    }
    for (int db = 0; db >= -60; db -= 20)
    {
        g.setColour (farge::linje.withAlpha (0.6f));
        g.drawHorizontalLine (roundToInt (dy ((float) db)), plott.getX(), plott.getRight());
        g.setColour (farge::dempet);
        g.drawText (String (db) + " dB", Rectangle<float> (alt.getX() + 4, dy ((float) db) - 8, 42, 16),
                    Justification::centredRight);
    }

    const auto baand = romsyntese::band (rom);
    const double t0 = pre + 0.006;
    const double bygg = rom.oppbygging * (0.3 + 0.7 * diff);
    auto kurve = [&] (double mult, float start)
    {
        Path pth;
        const double rtb = rt * mult;
        const int punkter = 240;
        for (int i = 0; i <= punkter; ++i)
        {
            const double t = t0 + (tMaks - t0) * i / punkter;
            const double tt = t - t0;
            float db = start - (float) (60.0 * tt / rtb);
            if (tt < bygg)
                db += 20.0f * std::log10 (jmax (1.0e-3, 0.5 - 0.5 * std::cos (MathConstants<double>::pi * tt / bygg)));
            const Point<float> pt (tx (t), dy (db));
            if (i == 0) pth.startNewSubPath (pt); else pth.lineTo (pt);
            if (db < dbMin && tt > bygg) break;
        }
        return pth;
    };

    const float startDb = -8.0f;
    auto midt = kurve (baand[romsyntese::bandMidt].mult, startDb);
    Path fyll (midt);
    fyll.lineTo (fyll.getCurrentPosition().x, plott.getBottom());
    fyll.lineTo (tx (t0), plott.getBottom());
    fyll.closeSubPath();
    g.setGradientFill (ColourGradient (farge::aksent.withAlpha (0.45f), 0, plott.getY(),
                                       farge::aksent.withAlpha (0.02f), 0, plott.getBottom(), false));
    g.fillPath (fyll);

    // Lav diffusion: vis halen som korn (enkeltekko) i stedet for glatt flate.
    if (diff < 0.75f)
    {
        Random rng (rom.seed);
        const int antall = (int) (40 + 360 * (1.0f - diff));
        g.setColour (farge::lys.withAlpha (0.25f + 0.4f * (1.0f - diff)));
        for (int i = 0; i < antall; ++i)
        {
            const double t = t0 + rng.nextDouble() * (tMaks - t0);
            const float topp = startDb - (float) (60.0 * (t - t0) / rt);
            if (topp < dbMin) continue;
            const float hoyde = rng.nextFloat() * (topp - dbMin);
            g.drawVerticalLine (roundToInt (tx (t)), dy (dbMin + hoyde), plott.getBottom());
        }
    }

    g.setColour (farge::bass);
    g.strokePath (kurve (baand[romsyntese::bandBass].mult, startDb - 2.0f), PathStrokeType (1.6f));
    g.setColour (farge::aksent);
    g.strokePath (midt, PathStrokeType (2.2f));
    Path diskant;
    PathStrokeType (1.6f).createDashedStroke (diskant, kurve (baand[romsyntese::bandDiskant].mult, startDb - 4.0f),
                                              std::array<float, 2> { 5.0f, 4.0f }.data(), 2);
    g.setColour (farge::diskant.withAlpha (0.85f));
    g.fillPath (diskant);

    const float erTopp = startDb + 4.0f + tidligeDb * 0.6f;
    for (const auto& f : romsyntese::refleksjoner (rom, pre))
    {
        const float db = erTopp + 20.0f * std::log10 (jmax (0.01f, std::abs (f.amp)));
        g.setColour (farge::lys.withAlpha (0.8f));
        g.drawLine (tx (f.tid), plott.getBottom(), tx (f.tid), dy (db), 1.0f + 2.0f * diff);
    }

    g.setColour (farge::lys.withAlpha (0.5f));
    const float px = tx (pre);
    for (float yy = plott.getY(); yy < plott.getBottom(); yy += 6.0f)
        g.drawVerticalLine (roundToInt (px), yy, jmin (yy + 3.0f, plott.getBottom()));

    // overskrift og forklaring
    auto topp = alt.reduced (14, 10).removeFromTop (22).withTrimmedLeft (236);
    g.setColour (farge::tekst);
    g.setFont (FontOptions (13.0f, Font::bold));
    g.drawText ("RT60 " + tall (rt, 1) + " s" + prikk + "Pre-delay "
                    + String (roundToInt (pre * 1000.0)) + " ms" + prikk + "Diffusion " + String (roundToInt (diff * 100)) + " %"
                    + prikk + "Mod " + String (roundToInt (p.getRawParameterValue ("mod_depth")->load())) + " %",
                topp, Justification::centredLeft);

    g.setFont (FontOptions (11.0f));
    auto forkl = topp.removeFromRight (300);
    auto merke = [&] (const String& txt, Colour c, bool stiplet, int b)
    {
        auto felt = forkl.removeFromLeft ((float) b);
        const float yy = felt.getCentreY();
        g.setColour (c);
        if (stiplet) { g.drawLine (felt.getX(), yy, felt.getX() + 5, yy, 1.6f); g.drawLine (felt.getX() + 9, yy, felt.getX() + 14, yy, 1.6f); }
        else g.drawLine (felt.getX(), yy, felt.getX() + 14, yy, 2.0f);
        g.setColour (farge::dempet);
        g.drawText (txt, felt.withTrimmedLeft (18), Justification::centredLeft);
    };
    merke ("Low", farge::bass, false, 62);
    merke ("Mid", farge::aksent, false, 58);
    merke ("High", farge::diskant, true, 64);
    g.setColour (farge::lys.withAlpha (0.8f));
    g.drawLine (forkl.getX() + 6, forkl.getY() + 5, forkl.getX() + 6, forkl.getBottom() - 5, 1.2f);
    g.setColour (farge::dempet);
    g.drawText ("Early ref.", forkl.withTrimmedLeft (12), Justification::centredLeft);
}

void KlangVisning::tegnFrekvensRutenett (Graphics& g, Rectangle<float> plott)
{
    const auto alt = getLocalBounds().toFloat();
    g.setFont (FontOptions (11.0f));
    for (double f : { 20.0, 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0, 20000.0 })
    {
        g.setColour (farge::linje.withAlpha (0.6f));
        g.drawVerticalLine (roundToInt (fx (f)), plott.getY(), plott.getBottom());
        g.setColour (farge::dempet);
        g.drawText (f >= 1000.0 ? String (roundToInt (f / 1000.0)) + "k" : String (roundToInt (f)),
                    Rectangle<float> (fx (f) - 20, plott.getBottom() + 4, 40, 16), Justification::centred);
    }
    for (int db = -12; db <= 12; db += 6)
    {
        g.setColour (db == 0 ? farge::linje.brighter (0.4f) : farge::linje.withAlpha (0.6f));
        g.drawHorizontalLine (roundToInt (eqY ((float) db)), plott.getX(), plott.getRight());
        g.setColour (farge::dempet);
        g.drawText ((db > 0 ? "+" : "") + String (db) + " dB", Rectangle<float> (alt.getX() + 4, eqY ((float) db) - 8, 42, 16),
                    Justification::centredRight);
    }
}

void KlangVisning::tegnHandtak (Graphics& g)
{
    const auto h = handtak();
    g.setFont (FontOptions (11.0f, Font::bold));
    for (size_t i = 0; i < h.size(); ++i)
    {
        const auto& ht = h[i];
        const bool fokus = (int) i == hover || (int) i == drar;
        const float d = fokus ? 13.0f : 10.0f;
        const auto c = ht.pos;
        if (fokus)
        {
            g.setColour (ht.farge.withAlpha (0.25f));
            g.fillEllipse (Rectangle<float> (d + 10, d + 10).withCentre (c));
        }
        if (ht.y == YType::range)
        {
            // Range: liten trekant som peker nedover (maks demping).
            Path t;
            t.addTriangle (c.x - d * 0.5f, c.y - d * 0.4f, c.x + d * 0.5f, c.y - d * 0.4f, c.x, c.y + d * 0.5f);
            g.setColour (ht.aktiv ? ht.farge : farge::dempet.withAlpha (0.6f));
            g.fillPath (t);
            g.setColour (farge::lys.withAlpha (0.8f));
            g.strokePath (t, PathStrokeType (1.0f));
            continue;
        }
        g.setColour (ht.aktiv ? farge::lys : farge::dempet.withAlpha (0.5f));
        g.fillEllipse (Rectangle<float> (d, d).withCentre (c));
        g.setColour (ht.y == YType::terskel ? ht.farge : farge::aksent2);
        g.drawEllipse (Rectangle<float> (d, d).withCentre (c), 1.6f);
        g.setColour (ht.aktiv || fokus ? farge::tekst : farge::dempet);
        g.drawText (ht.tekst, Rectangle<float> (c.x - 30, c.y - 23, 60, 14), Justification::centred);
    }
}

void KlangVisning::tegnEq (Graphics& g, Rectangle<float> plott)
{
    const auto alt = getLocalBounds().toFloat();
    tegnFrekvensRutenett (g, plott);
    tegnRta (g, plott, spInn, spVaat, true);

    Path kurve;
    const int punkter = 300;
    for (int i = 0; i <= punkter; ++i)
    {
        const double f = fMin * std::pow (fMaks / fMin, (double) i / punkter);
        const Point<float> pt (fx (f), eqY (FiolinProcessor::eqRespons (p, f)));
        if (i == 0) kurve.startNewSubPath (pt); else kurve.lineTo (pt);
    }
    Path fyll (kurve);
    fyll.lineTo (plott.getRight(), eqY (0.0f));
    fyll.lineTo (plott.getX(), eqY (0.0f));
    fyll.closeSubPath();
    g.setColour (farge::aksent.withAlpha (0.18f));
    g.fillPath (fyll);
    g.setColour (farge::aksent.withAlpha (0.25f));
    g.strokePath (kurve, PathStrokeType (6.0f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour (farge::aksent);
    g.strokePath (kurve, PathStrokeType (2.2f, PathStrokeType::curved, PathStrokeType::rounded));

    tegnHandtak (g);

    auto topp = alt.reduced (14, 10).removeFromTop (22).withTrimmedLeft (236);
    g.setColour (farge::tekst);
    g.setFont (FontOptions (13.0f, Font::bold));
    g.drawText ("Reverb EQ + RTA" + prikk + "drag the points", topp, Justification::centredLeft);

    g.setFont (FontOptions (11.0f));
    auto forkl = topp.removeFromRight (230);
    auto merke = [&] (const String& txt, Colour c, float tykk, int b)
    {
        auto felt = forkl.removeFromLeft ((float) b);
        g.setColour (c);
        g.drawLine (felt.getX(), felt.getCentreY(), felt.getX() + 14, felt.getCentreY(), tykk);
        g.setColour (farge::dempet);
        g.drawText (txt, felt.withTrimmedLeft (18), Justification::centredLeft);
    };
    merke ("Input", farge::diskant.withAlpha (0.6f), 1.2f, 70);
    merke ("Reverb", farge::aksent.withAlpha (0.6f), 5.0f, 80);
    merke ("EQ", farge::lys, 2.2f, 60);
}

void KlangVisning::tegnDyn (Graphics& g, Rectangle<float> plott)
{
    const auto alt = getLocalBounds().toFloat();
    tegnFrekvensRutenett (g, plott);
    // Uten tilt her, så terskelen kan leses mot dBFS-skalaen til høyre.
    tegnRta (g, plott, spFoerDyn, spVaat, false);

    const double fs = proc.hentRate();
    const int punkter = 300;
    auto lagKurve = [&] (auto&& dbVed)
    {
        Path pth;
        for (int i = 0; i <= punkter; ++i)
        {
            const double f = fMin * std::pow (fMaks / fMin, (double) i / punkter);
            const Point<float> pt (fx (f), eqY (dbVed (f)));
            if (i == 0) pth.startNewSubPath (pt); else pth.lineTo (pt);
        }
        return pth;
    };

    for (int b = 0; b < FiolinProcessor::antallDyn; ++b)
    {
        const auto c = b == 0 ? farge::aksent : farge::bass;
        const float f0 = v (FiolinProcessor::dynId (b, "freq")), q = v (FiolinProcessor::dynId (b, "q"));
        const float range = v (FiolinProcessor::dynId (b, "range"));
        const float terskel = v (FiolinProcessor::dynId (b, "thresh"));
        const bool aktiv = range > 0.05f;

        // Terskel: stiplet strek over båndbredden, og detektornivået som en
        // tykk strek (lyser opp når nivået er over terskelen).
        const float bw = jlimit (0.15f, 3.0f, 1.0f / q);
        const float x0 = fx (f0 * std::pow (2.0f, -bw * 0.5f)), x1 = fx (f0 * std::pow (2.0f, bw * 0.5f));
        Path strek, stiplet;
        strek.startNewSubPath (x0, rtaY (terskel));
        strek.lineTo (x1, rtaY (terskel));
        const float monster[] = { 4.0f, 3.0f };
        PathStrokeType (1.3f).createDashedStroke (stiplet, strek, monster, 2);
        g.setColour (c.withAlpha (aktiv ? 0.9f : 0.35f));
        g.fillPath (stiplet);
        if (aktiv)
        {
            const float nivaa = proc.hentDynNivaa (b);
            if (nivaa > rtaMin)
            {
                g.setColour ((nivaa > terskel ? farge::lys : c).withAlpha (0.7f));
                g.fillRect (Rectangle<float> (x0, rtaY (nivaa) - 1.5f, x1 - x0, 3.0f));
            }
            // Maks demping (Range) som stiplet kurve.
            Path maks, maksStiplet;
            maks = lagKurve ([&] (double f) { return FiolinProcessor::toppRespons (f, f0, q, -range, fs); });
            PathStrokeType (1.2f).createDashedStroke (maksStiplet, maks, monster, 2);
            g.setColour (c.withAlpha (0.55f));
            g.fillPath (maksStiplet);
        }
    }

    // Samlet demping akkurat nå (begge båndene), fylt.
    float gr[FiolinProcessor::antallDyn];
    for (int b = 0; b < FiolinProcessor::antallDyn; ++b) gr[b] = proc.hentDynGr (b);
    auto naa = lagKurve ([&] (double f)
    {
        float db = 0.0f;
        for (int b = 0; b < FiolinProcessor::antallDyn; ++b)
            db += FiolinProcessor::toppRespons (f, v (FiolinProcessor::dynId (b, "freq")), v (FiolinProcessor::dynId (b, "q")), gr[b], fs);
        return db;
    });
    Path fyll (naa);
    fyll.lineTo (plott.getRight(), eqY (0.0f));
    fyll.lineTo (plott.getX(), eqY (0.0f));
    fyll.closeSubPath();
    g.setColour (farge::lys.withAlpha (0.22f));
    g.fillPath (fyll);
    g.setColour (farge::lys);
    g.strokePath (naa, PathStrokeType (2.0f, PathStrokeType::curved, PathStrokeType::rounded));

    tegnHandtak (g);

    // Demping i dB ved hvert bånd
    g.setFont (FontOptions (11.0f, Font::bold));
    for (int b = 0; b < FiolinProcessor::antallDyn; ++b)
        if (v (FiolinProcessor::dynId (b, "range")) > 0.05f)
        {
            const float x = fx (v (FiolinProcessor::dynId (b, "freq")));
            const float y = eqY (-v (FiolinProcessor::dynId (b, "range"))) + 9.0f;
            g.setColour (farge::panel.withAlpha (0.8f));
            g.fillRoundedRectangle (Rectangle<float> (x - 24, y, 48, 15), 4.0f);
            g.setColour (farge::tekst);
            g.drawText (tall (gr[b], 1) + " dB", Rectangle<float> (x - 24, y, 48, 15), Justification::centred);
        }

    auto topp = alt.reduced (14, 10).removeFromTop (22).withTrimmedLeft (236);
    g.setColour (farge::tekst);
    g.setFont (FontOptions (13.0f, Font::bold));
    g.drawText ("Dynamic EQ + RTA" + prikk + "cuts only above Thresh", topp, Justification::centredLeft);

    g.setFont (FontOptions (11.0f));
    auto forkl = topp.removeFromRight (250);
    auto merke = [&] (const String& txt, Colour c, float tykk, int b)
    {
        auto felt = forkl.removeFromLeft ((float) b);
        g.setColour (c);
        g.drawLine (felt.getX(), felt.getCentreY(), felt.getX() + 14, felt.getCentreY(), tykk);
        g.setColour (farge::dempet);
        g.drawText (txt, felt.withTrimmedLeft (18), Justification::centredLeft);
    };
    merke ("Before", farge::diskant.withAlpha (0.6f), 1.2f, 76);
    merke ("After", farge::aksent.withAlpha (0.6f), 5.0f, 70);
    merke ("Gain", farge::lys, 2.0f, 60);
}

void KlangVisning::tegnRta (Graphics& g, Rectangle<float> plott, const Spektrum& linje,
                            const Spektrum& fyllSp, bool medTilt)
{
    // RTA i dBFS. Med tilt: 3 dB/okt rundt 1 kHz, så musikk ser omtrent flatt ut.
    const double nyq = proc.hentRate() * 0.5;
    auto tilt = [medTilt] (double f) { return medTilt ? (float) (3.0 * std::log2 (f / 1000.0)) : 0.0f; };

    g.setFont (FontOptions (10.5f));
    g.setColour (farge::dempet.withAlpha (0.8f));
    for (int db = 0; db >= -90; db -= 30)
        g.drawText (String (db), Rectangle<float> (plott.getRight() + 4, rtaY ((float) db) - 7, 40, 14),
                    Justification::centredLeft);

    auto lagKurve = [&] (const Spektrum& sp)
    {
        Path pth;
        const int punkter = 260;
        bool start = true;
        for (int i = 0; i <= punkter; ++i)
        {
            const double f = fMin * std::pow (fMaks / fMin, (double) i / punkter);
            if (f >= nyq) break;
            const Point<float> pt (fx (f), rtaY (sp.db (f) + tilt (f)));
            if (start) { pth.startNewSubPath (pt); start = false; } else pth.lineTo (pt);
        }
        return pth;
    };

    auto vaat = lagKurve (fyllSp);
    Path fyll (vaat);
    fyll.lineTo (fyll.getCurrentPosition().x, plott.getBottom());
    fyll.lineTo (plott.getX(), plott.getBottom());
    fyll.closeSubPath();
    g.setGradientFill (ColourGradient (farge::aksent.withAlpha (0.32f), 0, plott.getY(),
                                       farge::aksent2.withAlpha (0.05f), 0, plott.getBottom(), false));
    g.fillPath (fyll);
    g.setColour (farge::aksent.withAlpha (0.55f));
    g.strokePath (vaat, PathStrokeType (1.2f));

    g.setColour (farge::diskant.withAlpha (0.55f));
    g.strokePath (lagKurve (linje), PathStrokeType (1.2f));
}

//==============================================================================
FiolinEditor::FiolinEditor (FiolinProcessor& p)
    : AudioProcessorEditor (&p), prosessor (p), visning (p)
{
    setLookAndFeel (&utseende);
    addAndMakeVisible (innhold);
    innhold.setSize (basisBredde, basisHoyde);
    innhold.addAndMakeVisible (visning);
    visning.setTooltip ("DECAY: how the reverb dies away in the low, mid and high range. "
                        "Vertical lines are early reflections, the dotted line is the pre-delay.\n"
                        "EQ: real-time analyser (RTA) of the input (silver line) and the reverb after EQ (blue), "
                        "with the EQ curve on top. Drag LC/LS/HS/HC to change the EQ. The RTA uses a 3 dB/oct tilt; scale on the right in dBFS.\n"
                        "DYN EQ: the reverb before (silver) and after (blue) the dynamic EQ. Drag the circles (1, 2) to set "
                        "frequency and threshold, the triangles to set range. Mouse wheel = Q. Double-click = reset.");

    const auto navn = FiolinProcessor::romNavn();
    const String hjelp[] = {
        "Large hall: even and open. A great starting point for solo strings.",
        "Smaller, closer room with denser early reflections. Good for chamber music and fast passages.",
        "Darker, rounder hall where the highs die away faster. Good for slow, lyrical lines." };
    for (int i = 0; i < 3; ++i)
    {
        auto& b = romKnapper[(size_t) i];
        b.setButtonText (navn[i]);
        b.setTooltip (hjelp[i]);
        b.setConnectedEdges ((i > 0 ? Button::ConnectedOnLeft : 0) | (i < 2 ? Button::ConnectedOnRight : 0));
        b.onClick = [this, i] { velgRom (i); };
        innhold.addAndMakeVisible (b);
    }

    const char* fanenavn[] = { "DECAY", "EQ", "DYN EQ" };
    const KlangVisning::Modus fanemodus[] = { KlangVisning::Modus::decay, KlangVisning::Modus::eq, KlangVisning::Modus::dyn };
    for (int i = 0; i < 3; ++i)
    {
        auto& b = faner[(size_t) i];
        b.setButtonText (fanenavn[i]);
        b.setConnectedEdges ((i > 0 ? Button::ConnectedOnLeft : 0) | (i < 2 ? Button::ConnectedOnRight : 0));
        const auto m = fanemodus[i];
        b.onClick = [this, m] { velgVisning (m); };
        innhold.addAndMakeVisible (b);
    }

    using M = KlangVisning::Modus;
    lagKnott (decay, "lengde_s", "Decay",
              "Reverb time (RT60) in seconds, 0.3-10 s: the time it takes the tail to fall by 60 dB. "
              "Strings: 1.8-2.6 s. 5-10 s for cathedral or ambient spaces.", M::decay);
    lagKnott (predelay, "predelay_ms", "Pre-Delay",
              "Gap before the reverb starts. More pre-delay keeps attacks and bow strokes clear. Strings: 20-30 ms.", M::decay);
    lagKnott (diffusion, "diffusion", "Diffusion",
              "Echo density. High = smooth, dense tail (classic hall). Low = sparse, grainy tail where single echoes can be heard.", M::decay);
    lagKnott (tidlige, "tidlige", "Early Ref.",
              "Level of the early reflections, the first echoes from walls and ceiling. "
              "More gives presence and a sense of space, less gives a smoother tail.", M::decay);
    lagKnott (modDybde, "mod_depth", "Mod Depth",
              "How much the reverb tail moves: it glides slowly between three uncorrelated versions of the same room, "
              "like air moving in a hall. Livelier and less static, with no pitch change, chorus or phasing. 0 % = static.", M::decay);
    lagKnott (modRate, "mod_rate", "Mod Rate",
              "Speed of the movement (left and right move at their own, uneven rates). 0.2-0.5 Hz sounds natural; "
              "faster gives a more restless tail.", M::decay);
    lagKnott (lavkutt, "lavkutt", "Low Cut",
              "High-pass filter on the reverb (12 dB/oct). Keeps the low end clean. Off = 20 Hz.", M::eq);
    lagKnott (loFreq, "loshelf_freq", "Lo Freq", "Corner frequency of the low shelf.", M::eq);
    lagKnott (loGain, "loshelf_gain", "Lo Gain", "Boost or cut of the low shelf, +/-12 dB.", M::eq);
    lagKnott (hoykutt, "hoykutt", "High Cut",
              "Low-pass filter on the reverb (12 dB/oct). Lower settings sound softer and more distant. Off = 20 kHz.", M::eq);
    lagKnott (hiFreq, "hishelf_freq", "Hi Freq", "Corner frequency of the high shelf.", M::eq);
    lagKnott (hiGain, "hishelf_gain", "Hi Gain",
              "Boost or cut of the high shelf, +/-12 dB. A gentle cut tames bow noise in the tail.", M::eq);
    lagKnott (bredde, "bredde", "Width",
              "Stereo width of the reverb. 0 % = mono, 100 % = as designed, 150 % = extra wide.", M::decay);
    lagKnott (mix, "mix", "Mix",
              "Dry/wet balance. Up to 50 % the dry signal stays at full level, and the reverb rises gently "
              "(25 % = -10 dB, 50 % = full reverb). On an aux/send: set to 100 %.", M::decay);

    for (int b = 0; b < FiolinProcessor::antallDyn; ++b)
    {
        const String n (b + 1);
        lagKnott (dynFreq[(size_t) b], FiolinProcessor::dynId (b, "freq"), "Freq " + n,
                  "Centre frequency of dynamic band " + n + ".", M::dyn);
        lagKnott (dynQ[(size_t) b], FiolinProcessor::dynId (b, "q"), "Q " + n,
                  "Bandwidth of dynamic band " + n + ". Higher Q = narrower band.", M::dyn);
        lagKnott (dynThr[(size_t) b], FiolinProcessor::dynId (b, "thresh"), "Thresh " + n,
                  "Level (dBFS, RMS in the band) where band " + n + " starts to cut. "
                  "Only the parts of the reverb that get louder than this are turned down.", M::dyn);
        lagKnott (dynRange[(size_t) b], FiolinProcessor::dynId (b, "range"), "Range " + n,
                  "Maximum cut of band " + n + " (0 = off). Ratio 3:1, attack 8 ms, release 150 ms.", M::dyn);
    }

    setResizable (true, true);
    setResizeLimits (basisBredde * 3 / 4, basisHoyde * 3 / 4, basisBredde * 2, basisHoyde * 2);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) basisBredde / basisHoyde);
    setSize (basisBredde, basisHoyde);
    innhold.sendLookAndFeelChange();
    timerCallback();
    startTimerHz (15);
}

FiolinEditor::~FiolinEditor()
{
    setLookAndFeel (nullptr);
}

void FiolinEditor::lagKnott (Knott& k, const String& id, const String& navn, const String& hjelp,
                             KlangVisning::Modus v)
{
    k.slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (Slider::TextBoxBelow, false, 90, 20);
    k.slider.setTextBoxIsEditable (true);
    k.slider.setTooltip (hjelp + "\nDouble-click to reset. Click the value to type a number.");
    k.slider.setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
    k.slider.setMouseDragSensitivity (220);
    k.slider.onDragStart = [this, v] { velgVisning (v); };
    innhold.addAndMakeVisible (k.slider);
    k.kobling = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (prosessor.parametre, id, k.slider);
    if (auto* par = prosessor.parametre.getParameter (id))
        k.slider.setDoubleClickReturnValue (true, par->convertFrom0to1 (par->getDefaultValue()));
    k.navn.setText (navn.toUpperCase(), dontSendNotification);
    k.navn.setFont (FontOptions (11.0f, Font::bold).withKerningFactor (0.08f));
    k.navn.setJustificationType (Justification::centred);
    k.navn.setInterceptsMouseClicks (false, false);
    innhold.addAndMakeVisible (k.navn);
}

void FiolinEditor::velgRom (int rom)
{
    if (auto* par = prosessor.parametre.getParameter ("rom"))
    {
        par->beginChangeGesture();
        par->setValueNotifyingHost (par->convertTo0to1 ((float) rom));
        par->endChangeGesture();
    }
    velgVisning (KlangVisning::Modus::decay);
    timerCallback();
}

void FiolinEditor::velgVisning (KlangVisning::Modus m)
{
    using M = KlangVisning::Modus;
    visning.settModus (m);
    faner[0].setToggleState (m == M::decay, dontSendNotification);
    faner[1].setToggleState (m == M::eq, dontSendNotification);
    faner[2].setToggleState (m == M::dyn, dontSendNotification);
    if ((int) m == visteModus)
        return;
    visteModus = (int) m;
    // EQ-gruppen viser enten den faste EQ-en eller den dynamiske.
    const bool dynVis = m == M::dyn;
    for (auto* k : { &lavkutt, &loFreq, &loGain, &hoykutt, &hiFreq, &hiGain })
    {
        k->slider.setVisible (! dynVis);
        k->navn.setVisible (! dynVis);
    }
    for (int b = 0; b < FiolinProcessor::antallDyn; ++b)
        for (auto* k : { &dynFreq[(size_t) b], &dynQ[(size_t) b], &dynThr[(size_t) b], &dynRange[(size_t) b] })
        {
            k->slider.setVisible (dynVis);
            k->navn.setVisible (dynVis);
        }
    innhold.repaint();
}

void FiolinEditor::timerCallback()
{
    const int rom = (int) prosessor.parametre.getRawParameterValue ("rom")->load();
    for (int i = 0; i < 3; ++i)
        romKnapper[(size_t) i].setToggleState (i == rom, dontSendNotification);
    velgVisning (visning.hentModus());
}

void FiolinEditor::paint (Graphics& g)
{
    g.fillAll (farge::bakgrunn);
}

void FiolinEditor::resized()
{
    innhold.setTransform (AffineTransform::scale ((float) getWidth() / (float) basisBredde));
}

void FiolinEditor::plasser (Knott& k, Rectangle<int> celle, int d)
{
    const int hoyde = 16 + d + 20;
    auto r = celle.withSizeKeepingCentre (celle.getWidth(), hoyde);
    k.navn.setBounds (r.removeFromTop (16));
    k.slider.setBounds (r.withSizeKeepingCentre (jmax (d, 84), d + 20));
}

//==============================================================================
namespace
{
    const int visY = 80, visH = 196, gruppeY = 290;
    struct Gruppe { const char* navn; int x, b; };
    const Gruppe grupper[] = { { "REVERB", 24, 400 }, { "EQ", 436, 330 }, { "OUTPUT", 778, 190 } };
}

void FiolinEditor::Innhold::paint (Graphics& g)
{
    g.setGradientFill (ColourGradient (farge::panel.brighter (0.04f), 0, 0, farge::bakgrunn, 0, (float) getHeight(), false));
    g.fillAll();

    AttributedString navn;
    navn.append ("T1 ", FontOptions (30.0f, Font::bold), farge::lys);
    navn.append ("rev", FontOptions (30.0f), farge::tekst.withAlpha (0.85f));
    navn.append ("PRO", FontOptions (30.0f, Font::bold), farge::aksent);
    navn.setJustification (Justification::centredLeft);
    // Morset Labs-logo (samme serie som de andre pluginene)
    const auto logo = ImageCache::getFromMemory (LogoData::t1revprologo256_png, LogoData::t1revprologo256_pngSize);
    g.drawImage (logo, Rectangle<float> (20, 10, 54, 54), RectanglePlacement::centred);
    navn.draw (g, Rectangle<float> (84, 12, 320, 36));
    g.setColour (farge::dempet);
    g.setFont (FontOptions (11.5f).withKerningFactor (0.12f));
    g.drawText ("SYNTHETIC IR HALL" + prikk + "MORSET LABS", 86, 46, 360, 16, Justification::centredLeft);

    for (const auto& gr : grupper)
    {
        Rectangle<float> r ((float) gr.x, (float) gruppeY, (float) gr.b, (float) (getHeight() - gruppeY - 18));
        g.setColour (farge::panel.withAlpha (0.7f));
        g.fillRoundedRectangle (r, 10.0f);
        g.setColour (farge::linje);
        g.drawRoundedRectangle (r.reduced (0.5f), 10.0f, 1.0f);
        g.setColour (farge::aksent);
        g.setFont (FontOptions (11.0f, Font::bold).withKerningFactor (0.15f));
        const bool dynVis = ed.visning.hentModus() == KlangVisning::Modus::dyn;
        g.drawText (&gr == &grupper[1] && dynVis ? "DYNAMIC EQ" : gr.navn,
                    r.reduced (14, 8).removeFromTop (14), Justification::centredLeft);
    }
    // skillelinje mellom EQ-radene
    g.setColour (farge::linje.withAlpha (0.6f));
    const int midtY = gruppeY + 26 + (getHeight() - gruppeY - 18 - 26) / 2;
    g.drawHorizontalLine (midtY, (float) grupper[1].x + 14, (float) (grupper[1].x + grupper[1].b - 14));
    g.drawHorizontalLine (midtY, (float) grupper[0].x + 6 + 124 + 8, (float) (grupper[0].x + grupper[0].b - 14));
}

void FiolinEditor::Innhold::resized()
{
    auto& e = ed;
    auto rom = Rectangle<int> (getWidth() - 24 - 420, 20, 420, 34);
    for (auto& b : e.romKnapper)
        b.setBounds (rom.removeFromLeft (140));

    e.visning.setBounds (24, visY, getWidth() - 48, visH);
    auto fane = Rectangle<int> (24 + 12, visY + 10, 200, 24);
    e.faner[0].setBounds (fane.removeFromLeft (64));
    e.faner[1].setBounds (fane.removeFromLeft (52));
    e.faner[2].setBounds (fane.removeFromLeft (76));

    const int y = gruppeY + 26, h = getHeight() - y - 26;

    // REVERB: Decay stor, så tre små
    auto rv = Rectangle<int> (grupper[0].x + 6, y, grupper[0].b - 12, h);
    e.plasser (e.decay, rv.removeFromLeft (124), 110);
    // to rader: Pre-Delay / Diffusion / Early Ref. og Mod Depth / Mod Rate
    auto rvRad1 = rv.removeFromTop (rv.getHeight() / 2);
    const int kb3 = rvRad1.getWidth() / 3;
    e.plasser (e.predelay,  rvRad1.removeFromLeft (kb3), 54);
    e.plasser (e.diffusion, rvRad1.removeFromLeft (kb3), 54);
    e.plasser (e.tidlige,   rvRad1, 54);
    auto rvRad2 = rv.withSizeKeepingCentre (kb3 * 2, rv.getHeight());
    e.plasser (e.modDybde, rvRad2.removeFromLeft (kb3), 54);
    e.plasser (e.modRate,  rvRad2, 54);

    // EQ: to rader (lav / høy), kolonner CUT / FREQ / GAIN
    auto eq = Rectangle<int> (grupper[1].x + 6, y, grupper[1].b - 12, h);
    auto rad1 = eq.removeFromTop (eq.getHeight() / 2);
    auto rad2 = eq;
    const int kb = rad1.getWidth() / 3;
    e.plasser (e.lavkutt, rad1.removeFromLeft (kb), 54);
    e.plasser (e.loFreq,  rad1.removeFromLeft (kb), 54);
    e.plasser (e.loGain,  rad1, 54);
    e.plasser (e.hoykutt, rad2.removeFromLeft (kb), 54);
    e.plasser (e.hiFreq,  rad2.removeFromLeft (kb), 54);
    e.plasser (e.hiGain,  rad2, 54);

    // DYNAMIC EQ (samme plass): én rad per bånd, FREQ / Q / THRESH / RANGE
    auto dq = Rectangle<int> (grupper[1].x + 6, y, grupper[1].b - 12, h);
    for (int b = 0; b < FiolinProcessor::antallDyn; ++b)
    {
        auto rad = b == 0 ? dq.removeFromTop (dq.getHeight() / 2) : dq;
        const int kb4 = rad.getWidth() / 4;
        e.plasser (e.dynFreq[(size_t) b],  rad.removeFromLeft (kb4), 50);
        e.plasser (e.dynQ[(size_t) b],     rad.removeFromLeft (kb4), 50);
        e.plasser (e.dynThr[(size_t) b],   rad.removeFromLeft (kb4), 50);
        e.plasser (e.dynRange[(size_t) b], rad, 50);
    }

    // OUTPUT: Mix stor øverst, Width under
    auto ut = Rectangle<int> (grupper[2].x + 6, y, grupper[2].b - 12, h);
    e.plasser (e.mix, ut.removeFromTop (ut.getHeight() * 3 / 5), 96);
    e.plasser (e.bredde, ut, 54);
}
