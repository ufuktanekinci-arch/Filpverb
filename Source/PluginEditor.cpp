#include "PluginEditor.h"

using namespace juce;

namespace flip
{
//==============================================================================
LookAndFeel::LookAndFeel()
{
    setColour (ResizableWindow::backgroundColourId, col::bg);

    setColour (ComboBox::textColourId, col::text);
    setColour (ComboBox::backgroundColourId, Colour (0xff1b202b));
    setColour (ComboBox::outlineColourId, Colour (0xff2c3446));
    setColour (ComboBox::arrowColourId, col::dim);

    setColour (PopupMenu::backgroundColourId, Colour (0xff1a1e27));
    setColour (PopupMenu::textColourId, col::text);
    setColour (PopupMenu::highlightedBackgroundColourId, col::violet);
    setColour (PopupMenu::highlightedTextColourId, Colours::white);

    setColour (Slider::textBoxTextColourId, col::text);
    setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    setColour (Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    setColour (Slider::textBoxHighlightColourId, col::violet.withAlpha (0.4f));

    setColour (TextButton::buttonColourId, Colour (0xff1b202b));
    setColour (TextButton::buttonOnColourId, Colour (0xff222836));
    setColour (TextButton::textColourOffId, col::dim);
    setColour (TextButton::textColourOnId, col::text);
}

void LookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h,
                                    float pos, float start, float end, Slider& s)
{
    auto bounds = Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const float size = jmin (bounds.getWidth(), bounds.getHeight());
    auto area = bounds.withSizeKeepingCentre (size, size).reduced (4.0f);
    const float r = area.getWidth() * 0.5f;
    const auto c = area.getCentre();
    const float thick = 6.0f;
    const float arcR = r - thick * 0.5f;
    const auto accent = s.findColour (Slider::rotarySliderFillColourId);
    const float angle = start + pos * (end - start);

    Path track;
    track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, start, end, true);
    g.setColour (Colour (0xff232938));
    g.strokePath (track, PathStrokeType (thick, PathStrokeType::curved, PathStrokeType::rounded));

    if (pos > 0.002f)
    {
        Path val;
        val.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, start, angle, true);
        g.setColour (accent.withAlpha (0.22f));
        g.strokePath (val, PathStrokeType (thick + 6.0f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (accent);
        g.strokePath (val, PathStrokeType (thick, PathStrokeType::curved, PathStrokeType::rounded));
    }

    const float br = r - thick - 5.0f;
    g.setGradientFill (ColourGradient (Colour (0xff2b3142), c.x, c.y - br, Colour (0xff171b24), c.x, c.y + br, false));
    g.fillEllipse (c.x - br, c.y - br, br * 2.0f, br * 2.0f);
    g.setColour (Colour (0xff363e52));
    g.drawEllipse (c.x - br, c.y - br, br * 2.0f, br * 2.0f, 1.0f);

    Path pointer;
    pointer.startNewSubPath (c.x + br * 0.40f * std::sin (angle), c.y - br * 0.40f * std::cos (angle));
    pointer.lineTo (c.x + (br - 5.0f) * std::sin (angle), c.y - (br - 5.0f) * std::cos (angle));
    g.setColour (Colours::white);
    g.strokePath (pointer, PathStrokeType (3.0f, PathStrokeType::curved, PathStrokeType::rounded));
}

void LookAndFeel::drawToggleButton (Graphics& g, ToggleButton& b, bool highlighted, bool)
{
    const auto accent = b.findColour (ToggleButton::tickColourId);
    const auto bounds = b.getLocalBounds().toFloat();
    const float ph = 22.0f, pw = 40.0f;
    const Rectangle<float> pill (bounds.getX(), bounds.getCentreY() - ph * 0.5f, pw, ph);
    const bool on = b.getToggleState();

    Colour fill = on ? accent : Colour (0xff2a303d);
    if (highlighted)
        fill = fill.brighter (0.12f);

    if (on)
    {
        g.setColour (accent.withAlpha (0.22f));
        g.fillRoundedRectangle (pill.expanded (3.0f), ph * 0.5f + 3.0f);
    }

    g.setColour (fill);
    g.fillRoundedRectangle (pill, ph * 0.5f);

    const float tr = ph * 0.5f - 3.5f;
    const float cx = on ? pill.getRight() - ph * 0.5f : pill.getX() + ph * 0.5f;
    g.setColour (on ? Colours::white : col::dim);
    g.fillEllipse (cx - tr, pill.getCentreY() - tr, tr * 2.0f, tr * 2.0f);

    if (b.getButtonText().isNotEmpty())
    {
        g.setColour (on ? col::text : col::dim);
        g.setFont (Font (FontOptions (13.5f)));
        g.drawText (b.getButtonText(), (int) (pill.getRight() + 10.0f), 0,
                    b.getWidth() - (int) pw - 10, b.getHeight(), Justification::centredLeft);
    }
}

void LookAndFeel::drawComboBox (Graphics& g, int w, int h, bool down, int, int, int, int, ComboBox&)
{
    auto r = Rectangle<float> (0.0f, 0.0f, (float) w, (float) h).reduced (0.5f);
    g.setColour (down ? Colour (0xff222836) : Colour (0xff1b202b));
    g.fillRoundedRectangle (r, 10.0f);
    g.setColour (Colour (0xff2c3446));
    g.drawRoundedRectangle (r, 10.0f, 1.0f);

    Path p;
    const float cx = (float) w - 18.0f, cy = (float) h * 0.5f;
    p.startNewSubPath (cx - 4.5f, cy - 2.0f);
    p.lineTo (cx, cy + 2.5f);
    p.lineTo (cx + 4.5f, cy - 2.0f);
    g.setColour (col::dim);
    g.strokePath (p, PathStrokeType (1.8f, PathStrokeType::curved, PathStrokeType::rounded));
}

void LookAndFeel::positionComboBoxText (ComboBox& box, Label& label)
{
    label.setBounds (12, 1, box.getWidth() - 38, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
}

Font LookAndFeel::getComboBoxFont (ComboBox&) { return Font (FontOptions (14.5f)); }
Font LookAndFeel::getPopupMenuFont()          { return Font (FontOptions (14.5f)); }

//==============================================================================
StepRow::StepRow (AudioProcessorValueTreeState& apvts, const String& prefix, Colour a)
    : accent (a)
{
    for (int i = 0; i < 16; ++i)
        params[(size_t) i] = apvts.getParameter (prefix + String (i + 1));
}

Rectangle<float> StepRow::padBounds (int i) const
{
    constexpr float padGap = 4.0f, groupGap = 12.0f, top = 16.0f;
    const float padW = ((float) getWidth() - 3.0f * groupGap - 12.0f * padGap) / 16.0f;
    const int group = i / 4, inGroup = i % 4;
    const float x = (float) group * (4.0f * padW + 3.0f * padGap + groupGap) + (float) inGroup * (padW + padGap);
    return { x, top, padW, (float) getHeight() - top };
}

int StepRow::padAt (Point<float> p) const
{
    for (int i = 0; i < 16; ++i)
        if (padBounds (i).expanded (3.0f, 6.0f).contains (p))
            return i;
    return -1;
}

bool StepRow::getStep (int i) const
{
    auto* p = params[(size_t) i];
    return p != nullptr && p->getValue() > 0.5f;
}

void StepRow::setStep (int i, bool on)
{
    if (auto* p = params[(size_t) i])
    {
        if (getStep (i) == on)
            return;
        p->beginChangeGesture();
        p->setValueNotifyingHost (on ? 1.0f : 0.0f);
        p->endChangeGesture();
        repaint();
    }
}

void StepRow::setAll (bool on)
{
    for (int i = 0; i < 16; ++i)
        setStep (i, on);
}

void StepRow::setPlayhead (int step)
{
    if (step != playhead)
    {
        playhead = step;
        repaint();
    }
}

void StepRow::refresh()
{
    unsigned int bits = 0;
    for (int i = 0; i < 16; ++i)
        if (getStep (i))
            bits |= (1u << i);

    if (bits != lastBits)
    {
        lastBits = bits;
        repaint();
    }
}

void StepRow::mouseDown (const MouseEvent& e)
{
    const int i = padAt (e.position);
    lastDragged = i;
    if (i >= 0)
    {
        paintValue = ! getStep (i);
        setStep (i, paintValue);
    }
}

void StepRow::mouseDrag (const MouseEvent& e)
{
    const int i = padAt (e.position);
    if (i >= 0 && i != lastDragged)
    {
        lastDragged = i;
        setStep (i, paintValue);
    }
}

void StepRow::paint (Graphics& g)
{
    for (int i = 0; i < 16; ++i)
    {
        auto r = padBounds (i);
        const bool on = getStep (i);
        const bool head = (i == playhead);

        if (on)
        {
            g.setColour (accent.withAlpha (0.22f));
            g.fillRoundedRectangle (r.expanded (3.0f), 11.0f);
            g.setGradientFill (ColourGradient (accent.brighter (0.25f), r.getX(), r.getY(),
                                               accent.darker (0.35f), r.getX(), r.getBottom(), false));
            g.fillRoundedRectangle (r, 8.0f);
        }
        else
        {
            g.setColour (head ? Colour (0xff252b39) : col::padOff);
            g.fillRoundedRectangle (r, 8.0f);
            g.setColour (col::padLine);
            g.drawRoundedRectangle (r.reduced (0.5f), 8.0f, 1.0f);
        }

        if (head)
        {
            g.setColour (Colours::white.withAlpha (on ? 0.95f : 0.65f));
            g.drawRoundedRectangle (r.reduced (0.5f), 8.0f, 1.8f);
        }
    }

    g.setColour (col::dim);
    g.setFont (Font (FontOptions (11.5f)));
    for (int b = 0; b < 4; ++b)
        g.drawText (String (b + 1), (int) padBounds (b * 4).getX() + 2, 0, 30, 14, Justification::centredLeft);
}
} // namespace flip

//==============================================================================
FlipVerbAudioProcessorEditor::FlipVerbAudioProcessorEditor (FlipVerbAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      rvRow (p.apvts, "rvStep", flip::col::violet),
      ecRow (p.apvts, "ecStep", flip::col::cyan)
{
    using namespace flip;
    setLookAndFeel (&lnf);

    setupToggle (rvOn, "rvOn", col::violet);
    setupToggle (trOn, "trOn", col::pink);
    setupToggle (ecOn, "ecOn", col::cyan);
    setupToggle (ecPing, "ecPing", col::cyan, "Ping-Pong");

    setupCombo (rvLen, "rvLen");
    setupCombo (trRate, "trRate");
    setupCombo (ecTime, "ecTime");
    setupCombo (grid, "grid");

    setupKnob (rvMix,   U ("Menge"),       "rvMix",   col::violet);
    setupKnob (rvTone,  U ("Klang"),       "rvTone",  col::violet);
    setupKnob (trDepth, U ("Tiefe"),       "trDepth", col::pink);
    setupKnob (ecMix,   U ("Menge"),       "ecMix",   col::cyan);
    setupKnob (ecFb,    U ("Wiederholungen"), "ecFb", col::cyan);

    addAndMakeVisible (rvRow);
    addAndMakeVisible (ecRow);

    setupChip (rvAll,  [this] { rvRow.setAll (true); });
    setupChip (rvNone, [this] { rvRow.setAll (false); });
    setupChip (ecAll,  [this] { ecRow.setAll (true); });
    setupChip (ecNone, [this] { ecRow.setAll (false); });

    setSize (940, 640);
    setResizable (false, false);
    startTimerHz (30);
}

FlipVerbAudioProcessorEditor::~FlipVerbAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void FlipVerbAudioProcessorEditor::setupKnob (Knob& k, const String& title, const String& id, Colour accent)
{
    k.title = title;
    addAndMakeVisible (k.slider); // zuerst einhaengen, damit der Textkasten unser Design uebernimmt
    k.slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (Slider::TextBoxBelow, false, 90, 20);
    k.slider.setRotaryParameters (MathConstants<float>::pi * 1.2f, MathConstants<float>::pi * 2.8f, true);
    k.slider.setColour (Slider::rotarySliderFillColourId, accent);
    k.slider.setPopupDisplayEnabled (false, false, nullptr);
    k.att = std::make_unique<SliderAtt> (proc.apvts, id, k.slider);
}

void FlipVerbAudioProcessorEditor::setupCombo (Combo& c, const String& id)
{
    if (auto* choice = dynamic_cast<AudioParameterChoice*> (proc.apvts.getParameter (id)))
        c.box.addItemList (choice->choices, 1);
    addAndMakeVisible (c.box);
    c.att = std::make_unique<ComboAtt> (proc.apvts, id, c.box);
}

void FlipVerbAudioProcessorEditor::setupToggle (Toggle& t, const String& id, Colour accent, const String& text)
{
    t.button.setButtonText (text);
    t.button.setColour (ToggleButton::tickColourId, accent);
    t.button.setMouseCursor (MouseCursor::PointingHandCursor);
    addAndMakeVisible (t.button);
    t.att = std::make_unique<ButtonAtt> (proc.apvts, id, t.button);
}

void FlipVerbAudioProcessorEditor::setupChip (TextButton& b, std::function<void()> onClick)
{
    b.onClick = std::move (onClick);
    b.setMouseCursor (MouseCursor::PointingHandCursor);
    addAndMakeVisible (b);
}

void FlipVerbAudioProcessorEditor::timerCallback()
{
    const bool run = proc.uiRunning.load();
    const int step = proc.uiStep.load();
    rvRow.setPlayhead (run ? step : -1);
    ecRow.setPlayhead (run ? step : -1);
    rvRow.refresh();
    ecRow.refresh();
}

//==============================================================================
void FlipVerbAudioProcessorEditor::resized()
{
    const int W = getWidth();
    const int margin = 24, gap = 16;
    const int cardW = (W - 2 * margin - 2 * gap) / 3;
    const int cardY = 92, cardH = 292;

    for (int i = 0; i < 3; ++i)
        cards[i] = { margin + i * (cardW + gap), cardY, cardW, cardH };

    const int inner = cardW - 40;
    const int knobW = inner / 2;
    const int knobY = 140, knobH = 106;

    auto placeCard = [&] (int idx, Toggle& power, Combo& combo)
    {
        auto c = cards[idx];
        power.button.setBounds (c.getRight() - 20 - 44, c.getY() + 16, 44, 24);
        combo.box.setBounds (c.getX() + 20, c.getY() + 84, inner, 30);
    };

    placeCard (0, rvOn, rvLen);
    placeCard (1, trOn, trRate);
    placeCard (2, ecOn, ecTime);

    rvMix.slider.setBounds   (cards[0].getX() + 20,         cards[0].getY() + knobY, knobW, knobH);
    rvTone.slider.setBounds  (cards[0].getX() + 20 + knobW, cards[0].getY() + knobY, knobW, knobH);
    trDepth.slider.setBounds (cards[1].getX() + 20 + knobW / 2, cards[1].getY() + knobY, knobW, knobH);
    ecMix.slider.setBounds   (cards[2].getX() + 20,         cards[2].getY() + knobY, knobW, knobH);
    ecFb.slider.setBounds    (cards[2].getX() + 20 + knobW, cards[2].getY() + knobY, knobW, knobH);

    ecPing.button.setBounds (cards[2].getX() + 20, cards[2].getY() + 256, 150, 24);

    // Felder-Bereich
    stepsPanel = { margin, cardY + cardH + 16, W - 2 * margin, 216 };
    grid.box.setBounds (stepsPanel.getRight() - 20 - 190, stepsPanel.getY() + 14, 190, 28);

    const int labelW = 120, chipsW = 116, rowH = 56;
    const int padsX = stepsPanel.getX() + 20 + labelW;
    const int padsW = stepsPanel.getWidth() - 40 - labelW - chipsW - 12;
    const int row1Y = stepsPanel.getY() + 70;
    const int row2Y = stepsPanel.getY() + 142;

    rvRowArea = { stepsPanel.getX() + 20, row1Y, stepsPanel.getWidth() - 40, rowH };
    ecRowArea = { stepsPanel.getX() + 20, row2Y, stepsPanel.getWidth() - 40, rowH };

    rvRow.setBounds (padsX, row1Y, padsW, rowH);
    ecRow.setBounds (padsX, row2Y, padsW, rowH);

    const int chipsX = stepsPanel.getRight() - 20 - chipsW;
    rvAll.setBounds  (chipsX,      row1Y + 21, 54, 26);
    rvNone.setBounds (chipsX + 62, row1Y + 21, 54, 26);
    ecAll.setBounds  (chipsX,      row2Y + 21, 54, 26);
    ecNone.setBounds (chipsX + 62, row2Y + 21, 54, 26);
}

void FlipVerbAudioProcessorEditor::paint (Graphics& g)
{
    using namespace flip;
    const int W = getWidth(), H = getHeight();

    g.fillAll (col::bg);
    g.setGradientFill (ColourGradient (col::violet.withAlpha (0.16f), 0.0f, 0.0f,
                                       Colours::transparentBlack, (float) W * 0.7f, (float) H * 0.55f, true));
    g.fillRect (0, 0, W, H);
    g.setGradientFill (ColourGradient (col::cyan.withAlpha (0.09f), (float) W, (float) H,
                                       Colours::transparentBlack, (float) W * 0.4f, (float) H * 0.5f, true));
    g.fillRect (0, 0, W, H);

    // Kopf
    g.setGradientFill (ColourGradient (col::violet, 24.0f, 0.0f, col::cyan, 190.0f, 0.0f, false));
    g.setFont (Font (FontOptions (34.0f, Font::bold)));
    g.drawText ("FlipVerb", 24, 18, 260, 40, Justification::centredLeft);

    g.setColour (col::dim);
    g.setFont (Font (FontOptions (14.0f)));
    g.drawText (U ("Rückwärts-Hall  ·  Tremolo  ·  Echo-Würfe – einfach & schnell"), 26, 56, 600, 20, Justification::centredLeft);

    // Karten
    struct CardInfo { const char* title; const char* sub; const char* foot; Colour accent; const char* comboLabel; };
    const CardInfo info[3] = {
        { "RÜCKWÄRTS-HALL", "Der Hall baut sich VOR dem Ton auf", "FL Studio gleicht die Verzögerung aus", col::violet, "Länge des Aufbaus" },
        { "TREMOLO",        "Lässt den Rückwärts-Hall pulsieren", "Wirkt nur auf den Rückwärts-Hall",      col::pink,   "Tempo des Pulsierens" },
        { "ECHO-WURF",      "Echo nur an den gewählten Stellen",  "",                                           col::cyan,   "Abstand der Echos" }
    };

    for (int i = 0; i < 3; ++i)
    {
        auto c = cards[i].toFloat();
        g.setColour (col::card);
        g.fillRoundedRectangle (c, 18.0f);
        g.setColour (col::cardLine);
        g.drawRoundedRectangle (c.reduced (0.5f), 18.0f, 1.0f);

        const int x0 = cards[i].getX() + 20, y0 = cards[i].getY();

        g.setColour (info[i].accent);
        g.fillEllipse ((float) x0, (float) y0 + 22.0f, 10.0f, 10.0f);

        g.setColour (col::text);
        g.setFont (Font (FontOptions (16.0f, Font::bold)));
        g.drawText (U (info[i].title), x0 + 18, y0 + 14, 170, 26, Justification::centredLeft);

        g.setColour (col::dim);
        g.setFont (Font (FontOptions (12.5f)));
        g.drawText (U (info[i].sub), x0, y0 + 42, cards[i].getWidth() - 40, 18, Justification::centredLeft);

        g.setFont (Font (FontOptions (12.0f)));
        g.drawText (U (info[i].comboLabel), x0, y0 + 66, cards[i].getWidth() - 40, 16, Justification::centredLeft);

        if (info[i].foot[0] != '\0')
        {
            g.setFont (Font (FontOptions (11.5f)));
            g.drawText (U (info[i].foot), x0, y0 + 258, cards[i].getWidth() - 40, 18, Justification::centredLeft);
        }
    }

    // Knopf-Beschriftungen
    auto knobTitle = [&] (const Knob& k)
    {
        g.setColour (col::dim);
        g.setFont (Font (FontOptions (12.5f)));
        g.drawText (k.title, k.slider.getX(), k.slider.getY() - 16, k.slider.getWidth(), 14, Justification::centred);
    };
    knobTitle (rvMix); knobTitle (rvTone); knobTitle (trDepth); knobTitle (ecMix); knobTitle (ecFb);

    // Felder-Bereich
    auto sp = stepsPanel.toFloat();
    g.setColour (col::card);
    g.fillRoundedRectangle (sp, 18.0f);
    g.setColour (col::cardLine);
    g.drawRoundedRectangle (sp.reduced (0.5f), 18.0f, 1.0f);

    g.setColour (col::text);
    g.setFont (Font (FontOptions (16.0f, Font::bold)));
    g.drawText (U ("WO SOLL ES WIRKEN?"), stepsPanel.getX() + 20, stepsPanel.getY() + 12, 300, 24, Justification::centredLeft);

    g.setColour (col::dim);
    g.setFont (Font (FontOptions (12.5f)));
    g.drawText (U ("Klicken oder drüberziehen. Leuchtend = Effekt ist dort aktiv. Wähle die Felder, in denen dein Ton oder Wort liegt."),
                stepsPanel.getX() + 20, stepsPanel.getY() + 38, stepsPanel.getWidth() - 260, 18, Justification::centredLeft);

    g.drawText (U ("Ein Feld ="), grid.box.getX() - 84, grid.box.getY(), 76, grid.box.getHeight(), Justification::centredRight);

    auto rowLabel = [&] (juce::Rectangle<int> area, const char* text, Colour accent)
    {
        g.setColour (accent);
        g.fillEllipse ((float) area.getX(), (float) area.getY() + 29.0f, 10.0f, 10.0f);
        g.setColour (col::text);
        g.setFont (Font (FontOptions (14.0f, Font::bold)));
        g.drawText (U (text), area.getX() + 18, area.getY() + 16, 100, 40, Justification::centredLeft);
    };
    rowLabel (rvRowArea, "Rückwärts-Hall", col::violet);
    rowLabel (ecRowArea, "Echo-Wurf", col::cyan);
}
