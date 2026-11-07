#pragma once

#include "PluginProcessor.h"
#include <array>

namespace flip
{
namespace col
{
    inline const juce::Colour bg       { 0xff0c0e13 };
    inline const juce::Colour card     { 0xff151922 };
    inline const juce::Colour cardLine { 0xff222836 };
    inline const juce::Colour padOff   { 0xff1b202b };
    inline const juce::Colour padLine  { 0xff2a3142 };
    inline const juce::Colour text     { 0xffeef1f8 };
    inline const juce::Colour dim      { 0xff8089a0 };
    inline const juce::Colour violet   { 0xff8b5cf6 };
    inline const juce::Colour pink     { 0xfff472b6 };
    inline const juce::Colour cyan     { 0xff22d3ee };
}

inline juce::String U (const char* s) { return juce::String::fromUTF8 (s); }

//==============================================================================
class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    LookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStart, float rotaryEnd, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawComboBox (juce::Graphics&, int width, int height, bool isDown,
                       int bx, int by, int bw, int bh, juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;
};

//==============================================================================
/** 16 Felder zum Anklicken (oder Ueberziehen). Leuchtend = Effekt wirkt an dieser Stelle. */
class StepRow : public juce::Component
{
public:
    StepRow (juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix, juce::Colour accent);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

    void setPlayhead (int step);
    void setAll (bool on);
    void refresh();

private:
    juce::Rectangle<float> padBounds (int i) const;
    int padAt (juce::Point<float> p) const;
    bool getStep (int i) const;
    void setStep (int i, bool on);

    std::array<juce::RangedAudioParameter*, 16> params {};
    juce::Colour accent;
    int playhead = -1;
    unsigned int lastBits = 0xffffffffu;
    int lastDragged = -1;
    bool paintValue = true;
};

} // namespace flip

//==============================================================================
class FlipVerbAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     private juce::Timer
{
public:
    explicit FlipVerbAudioProcessorEditor (FlipVerbAudioProcessor&);
    ~FlipVerbAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct Knob  { juce::Slider slider; juce::String title; std::unique_ptr<SliderAtt> att; };
    struct Combo { juce::ComboBox box;  std::unique_ptr<ComboAtt> att; };
    struct Toggle { juce::ToggleButton button; std::unique_ptr<ButtonAtt> att; };

    void timerCallback() override;
    void setupKnob (Knob&, const juce::String& title, const juce::String& id, juce::Colour accent);
    void setupCombo (Combo&, const juce::String& id);
    void setupToggle (Toggle&, const juce::String& id, juce::Colour accent, const juce::String& text = {});
    void setupChip (juce::TextButton&, std::function<void()> onClick);

    FlipVerbAudioProcessor& proc;
    flip::LookAndFeel lnf;

    Toggle rvOn, trOn, ecOn, ecPing;
    Combo rvLen, trRate, ecTime, grid;
    Knob rvMix, rvTone, trDepth, ecMix, ecFb;

    flip::StepRow rvRow, ecRow;
    juce::TextButton rvAll { flip::U ("Alle") }, rvNone { flip::U ("Keine") },
                     ecAll { flip::U ("Alle") }, ecNone { flip::U ("Keine") };

    juce::Rectangle<int> cards[3], stepsPanel, rvRowArea, ecRowArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FlipVerbAudioProcessorEditor)
};
