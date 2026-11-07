#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>

/**
    FlipVerb - Rueckwaerts-Hall, Tremolo auf dem Hall und Echo-Wuerfe
    an frei waehlbaren Stellen (16 Felder pro Raster).

    So funktioniert der Rueckwaerts-Hall:
    Das Signal wird mit einem *zeitlich umgedrehten* Hall-Schweif gefaltet
    (leise anschwellend, am Ende abrupt). Gleichzeitig wird das Originalsignal
    um genau diese Laenge verzoegert. Dadurch baut sich der Hall VOR dem
    Originalton auf. Die Verzoegerung meldet das Plugin als Latenz an die DAW,
    FL Studio gleicht sie automatisch aus.
*/
class FlipVerbAudioProcessor : public juce::AudioProcessor,
                               private juce::Timer
{
public:
    static constexpr int numSteps = 16;

    FlipVerbAudioProcessor();
    ~FlipVerbAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "FlipVerb"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override;

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    /** Neuen Rueckwaerts-Hall-Schweif erzeugen, wenn Laenge/Tempo/Samplerate sich geaendert haben.
        Wird vom Timer (Message-Thread) aufgerufen, nie vom Audio-Thread. */
    void refreshImpulseResponse (bool force = false);

    int getDryDelaySamples() const noexcept { return dryDelay.load(); }

    juce::AudioProcessorValueTreeState apvts;

    // Fuer die Oberflaeche: aktuelles Feld (0..15) der Abspielposition
    std::atomic<int>  uiStep    { 0 };
    std::atomic<bool> uiRunning { false };

private:
    void timerCallback() override { refreshImpulseResponse (false); }

    // --- Parameter-Zeiger (Audio-Thread-sicher) ---
    std::atomic<float>* pRvOn    = nullptr;
    std::atomic<float>* pRvLen   = nullptr;
    std::atomic<float>* pRvMix   = nullptr;
    std::atomic<float>* pRvTone  = nullptr;
    std::atomic<float>* pTrOn    = nullptr;
    std::atomic<float>* pTrRate  = nullptr;
    std::atomic<float>* pTrDepth = nullptr;
    std::atomic<float>* pEcOn    = nullptr;
    std::atomic<float>* pEcTime  = nullptr;
    std::atomic<float>* pEcMix   = nullptr;
    std::atomic<float>* pEcFb    = nullptr;
    std::atomic<float>* pEcPing  = nullptr;
    std::atomic<float>* pGrid    = nullptr;
    std::array<std::atomic<float>*, numSteps> pRvStep {};
    std::array<std::atomic<float>*, numSteps> pEcStep {};

    // --- Rueckwaerts-Hall ---
    juce::dsp::Convolution conv;
    juce::AudioBuffer<float> wetBuf;
    juce::AudioBuffer<float> dryRing;
    int dryWritePos = 0;
    std::atomic<int> dryDelay { 0 };
    std::atomic<double> hostBpm { 120.0 };

    juce::CriticalSection irLock;
    int    irLenChoice = -1;
    int    irSamples   = 0;
    double irSampleRate = 0.0;

    // --- Echo ---
    juce::AudioBuffer<float> echoBuf;
    int echoWritePos = 0;

    // --- Zustaende ---
    double sr = 0.0;
    int maxBlock = 512;
    double freePpq = 0.0;

    float gateRv = 0.0f, gateEc = 0.0f;
    float toneLp1[2] {}, toneLp2[2] {};
    float fbLp[2] {};

    juce::SmoothedValue<float> trDepthSm, rvOutSm, ecOutSm, echoDelaySm;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FlipVerbAudioProcessor)
};
