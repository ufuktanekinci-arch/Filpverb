#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>
#include <vector>

namespace
{
constexpr double kMaxRvSeconds   = 6.0;
constexpr double kMaxEchoSeconds = 4.0;
constexpr float  kWetScale       = 1.6f;   // Grundlautstaerke des Halls bei "Menge" = 100 %

constexpr double kRvLenBeats[]  = { 1.0, 2.0, 4.0 };
constexpr double kTrRateBeats[] = { 1.0, 0.5, 0.25, 1.0 / 3.0 };
constexpr double kEcTimeBeats[] = { 0.25, 0.5, 0.75, 1.0, 1.5, 2.0 };

inline juce::String U (const char* s) { return juce::String::fromUTF8 (s); }

/** Feld-Nummer (0..15) fuer eine Position in Vierteln (ppq). */
inline int stepIndexAt (double ppqPos, double stepBeats)
{
    const double s = std::floor (ppqPos / stepBeats);
    int idx = (int) std::fmod (s, 16.0);
    if (idx < 0)
        idx += 16;
    return idx;
}

/** Zeitlich umgedrehter Hall-Schweif (stereo, unterschiedliches Rauschen links/rechts).
    Vorwaerts: laut -> leise, hell -> dunkel. Umgedreht: leise & dunkel -> laut & hell, dann Schluss. */
juce::AudioBuffer<float> makeReverseIR (int n, double sr)
{
    juce::AudioBuffer<float> ir (2, n);
    std::vector<float> h ((size_t) n);

    const float srScale = (float) (48000.0 / sr);

    for (int ch = 0; ch < 2; ++ch)
    {
        juce::Random rng (0x5eed1234 + 7919 * ch);
        float lp = 0.0f;
        double energy = 0.0;

        for (int j = 0; j < n; ++j)
        {
            const float t   = (float) j / (float) n;
            const float env = std::exp (-4.0f * t);
            const float a   = juce::jmin (0.99f, (0.85f - 0.78f * std::sqrt (t)) * srScale);

            lp += a * ((rng.nextFloat() * 2.0f - 1.0f) - lp);
            const float v = lp * env;
            h[(size_t) j] = v;
            energy += (double) v * (double) v;
        }

        const float scale = energy > 0.0 ? (float) (1.0 / std::sqrt (energy)) : 0.0f;
        float* d = ir.getWritePointer (ch);

        for (int i = 0; i < n; ++i)
            d[i] = h[(size_t) (n - 1 - i)] * scale;

        // winziges Ausblenden am Ende, damit es nicht knackt
        const int fade = juce::jmax (1, juce::jmin (n / 4, (int) (0.001 * sr)));
        for (int k = 0; k < fade; ++k)
            d[n - 1 - k] *= (float) k / (float) fade;
    }

    return ir;
}
} // namespace

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout FlipVerbAudioProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto pct = [] (float v, int) { return String (roundToInt (v * 100.0f)) + " %"; };
    auto pctFrom = [] (const String& s) { return s.getFloatValue() / 100.0f; };
    auto pctAttr = [&]() { return AudioParameterFloatAttributes().withStringFromValueFunction (pct)
                                                                 .withValueFromStringFunction (pctFrom); };

    // Rueckwaerts-Hall
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "rvOn", 1 }, U ("Rückwärts-Hall an"), true));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "rvLen", 1 }, U ("Hall Länge"),
                    StringArray { U ("1 Schlag"), U ("2 Schläge"), U ("1 Takt") }, 1));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "rvMix", 1 }, U ("Hall Menge"),
                    NormalisableRange<float> (0.0f, 1.0f), 0.6f, pctAttr()));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "rvTone", 1 }, U ("Hall Klang"),
                    NormalisableRange<float> (0.0f, 1.0f), 0.7f,
                    AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
                    {
                        return v < 0.34f ? String (juce::CharPointer_UTF8 ("dunkel"))
                             : v < 0.67f ? String (juce::CharPointer_UTF8 ("warm"))
                                         : String (juce::CharPointer_UTF8 ("hell"));
                    })));

    // Tremolo
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "trOn", 1 }, U ("Tremolo an"), false));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "trRate", 1 }, U ("Tremolo Tempo"),
                    StringArray { "1/4", "1/8", "1/16", U ("1/8 Triole") }, 1));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "trDepth", 1 }, U ("Tremolo Tiefe"),
                    NormalisableRange<float> (0.0f, 1.0f), 0.8f, pctAttr()));

    // Echo-Wurf
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "ecOn", 1 }, U ("Echo-Wurf an"), true));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "ecTime", 1 }, U ("Echo Abstand"),
                    StringArray { "1/16", "1/8", U ("1/8 punktiert"), "1/4", U ("1/4 punktiert"), "1/2" }, 2));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "ecMix", 1 }, U ("Echo Menge"),
                    NormalisableRange<float> (0.0f, 1.0f), 0.6f, pctAttr()));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "ecFb", 1 }, U ("Echo Wiederholungen"),
                    NormalisableRange<float> (0.0f, 0.9f), 0.45f,
                    AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
                    {
                        return String (roundToInt (v / 0.9f * 100.0f)) + " %";
                    }).withValueFromStringFunction ([] (const String& s) { return s.getFloatValue() / 100.0f * 0.9f; })));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "ecPing", 1 }, "Ping-Pong", false));

    // Raster
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "grid", 1 }, U ("Raster"),
                    StringArray { U ("1/16-Note (1 Takt)"), U ("1/8-Note (2 Takte)") }, 0));

    // 16 Felder fuer Hall und Echo
    for (int i = 0; i < numSteps; ++i)
    {
        const bool rvDefault = (i == 0 || i == 8);
        const bool ecDefault = (i == 7 || i == 15);
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { "rvStep" + String (i + 1), 1 },
                                                         U ("Hall Feld ") + String (i + 1), rvDefault));
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { "ecStep" + String (i + 1), 1 },
                                                         U ("Echo Feld ") + String (i + 1), ecDefault));
    }

    return layout;
}

//==============================================================================
FlipVerbAudioProcessor::FlipVerbAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout()),
      conv (juce::dsp::Convolution::NonUniform { 512 })
{
    pRvOn    = apvts.getRawParameterValue ("rvOn");
    pRvLen   = apvts.getRawParameterValue ("rvLen");
    pRvMix   = apvts.getRawParameterValue ("rvMix");
    pRvTone  = apvts.getRawParameterValue ("rvTone");
    pTrOn    = apvts.getRawParameterValue ("trOn");
    pTrRate  = apvts.getRawParameterValue ("trRate");
    pTrDepth = apvts.getRawParameterValue ("trDepth");
    pEcOn    = apvts.getRawParameterValue ("ecOn");
    pEcTime  = apvts.getRawParameterValue ("ecTime");
    pEcMix   = apvts.getRawParameterValue ("ecMix");
    pEcFb    = apvts.getRawParameterValue ("ecFb");
    pEcPing  = apvts.getRawParameterValue ("ecPing");
    pGrid    = apvts.getRawParameterValue ("grid");

    for (int i = 0; i < numSteps; ++i)
    {
        pRvStep[(size_t) i] = apvts.getRawParameterValue ("rvStep" + juce::String (i + 1));
        pEcStep[(size_t) i] = apvts.getRawParameterValue ("ecStep" + juce::String (i + 1));
    }

    startTimerHz (10);
}

FlipVerbAudioProcessor::~FlipVerbAudioProcessor()
{
    stopTimer();
}

double FlipVerbAudioProcessor::getTailLengthSeconds() const
{
    return sr > 0.0 ? (double) dryDelay.load() / sr + 4.0 : 4.0;
}

bool FlipVerbAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet()  == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

//==============================================================================
void FlipVerbAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    maxBlock = juce::jmax (1, samplesPerBlock);

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) maxBlock, 2 };
    conv.prepare (spec);
    conv.reset();

    wetBuf.setSize (2, maxBlock);
    wetBuf.clear();

    dryRing.setSize (2, (int) (kMaxRvSeconds * sr) + 8192);
    dryRing.clear();
    dryWritePos = 0;

    echoBuf.setSize (2, (int) (kMaxEchoSeconds * sr) + 8);
    echoBuf.clear();
    echoWritePos = 0;

    gateRv = gateEc = 0.0f;
    for (int c = 0; c < 2; ++c)
        toneLp1[c] = toneLp2[c] = fbLp[c] = 0.0f;

    const bool trOn = pTrOn->load() > 0.5f;
    const bool rvOn = pRvOn->load() > 0.5f;
    const bool ecOn = pEcOn->load() > 0.5f;

    trDepthSm.reset (sr, 0.03);
    trDepthSm.setCurrentAndTargetValue (trOn ? pTrDepth->load() : 0.0f);
    rvOutSm.reset (sr, 0.03);
    rvOutSm.setCurrentAndTargetValue (rvOn ? 1.0f : 0.0f);
    ecOutSm.reset (sr, 0.03);
    ecOutSm.setCurrentAndTargetValue (ecOn ? pEcMix->load() : 0.0f);
    echoDelaySm.reset (sr, 0.08);
    echoDelaySm.setCurrentAndTargetValue ((float) (kEcTimeBeats[(int) pEcTime->load()] * 60.0 / hostBpm.load() * sr));

    freePpq = 0.0;
    refreshImpulseResponse (true);
}

void FlipVerbAudioProcessor::refreshImpulseResponse (bool force)
{
    const juce::ScopedLock sl (irLock);

    if (sr <= 0.0)
        return;

    const int lenChoice = juce::jlimit (0, 2, (int) pRvLen->load());
    const double bpm = juce::jlimit (20.0, 400.0, hostBpm.load());
    const double seconds = juce::jlimit (0.05, kMaxRvSeconds, kRvLenBeats[lenChoice] * 60.0 / bpm);
    const int n = (int) std::lround (seconds * sr);

    const bool changed = force
                      || lenChoice != irLenChoice
                      || sr != irSampleRate
                      || std::abs (n - irSamples) > juce::jmax (1, irSamples / 100);

    if (! changed)
        return;

    irLenChoice  = lenChoice;
    irSampleRate = sr;
    irSamples    = n;

    conv.loadImpulseResponse (makeReverseIR (n, sr), sr,
                              juce::dsp::Convolution::Stereo::yes,
                              juce::dsp::Convolution::Trim::no,
                              juce::dsp::Convolution::Normalise::no);

    const int total = n + conv.getLatency();
    dryDelay.store (total);
    setLatencySamples (total);
}

//==============================================================================
void FlipVerbAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2 || sr <= 0.0 || numSamples <= 0)
        return;

    // ---- Position und Tempo vom Host ---------------------------------------
    double bpm = 120.0;
    double ppq = 0.0;
    bool playing = false, havePpq = false;

    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm())
                bpm = *b;
            playing = pos->getIsPlaying();
            if (auto p = pos->getPpqPosition())
            {
                ppq = *p;
                havePpq = true;
            }
        }
    }

    if (! (bpm >= 20.0 && bpm <= 400.0))
        bpm = 120.0;
    hostBpm.store (bpm);

    const double beatsPerSample = bpm / 60.0 / sr;

    if (playing && havePpq)
        freePpq = ppq + numSamples * beatsPerSample;
    else
    {
        // Host steht (oder liefert keine Position): intern weiterzaehlen, damit der Effekt trotzdem arbeitet
        ppq = freePpq;
        freePpq += numSamples * beatsPerSample;
    }

    // ---- Parameter ------------------------------------------------------------
    const bool rvOn = pRvOn->load() > 0.5f;
    const bool trOn = pTrOn->load() > 0.5f;
    const bool ecOn = pEcOn->load() > 0.5f;
    const bool ping = pEcPing->load() > 0.5f;

    const float rvMix   = pRvMix->load();
    const float rvTone  = pRvTone->load();
    const float trDepth = pTrDepth->load();
    const float ecMix   = pEcMix->load();
    const float ecFb    = pEcFb->load();

    const double trBeats = kTrRateBeats[juce::jlimit (0, 3, (int) pTrRate->load())];
    const double ecBeats = kEcTimeBeats[juce::jlimit (0, 5, (int) pEcTime->load())];
    const double stepBeats = ((int) pGrid->load() == 0) ? 0.25 : 0.5;

    bool rvActive[numSteps], ecActive[numSteps];
    for (int i = 0; i < numSteps; ++i)
    {
        rvActive[i] = pRvStep[(size_t) i]->load() > 0.5f;
        ecActive[i] = pEcStep[(size_t) i]->load() > 0.5f;
    }

    uiStep.store (stepIndexAt (ppq, stepBeats));
    uiRunning.store (playing);

    trDepthSm.setTargetValue (trOn ? trDepth : 0.0f);
    rvOutSm.setTargetValue (rvOn ? 1.0f : 0.0f);
    ecOutSm.setTargetValue (ecOn ? ecMix : 0.0f);

    const int echoSize = echoBuf.getNumSamples();
    echoDelaySm.setTargetValue ((float) juce::jlimit (2.0, (double) echoSize - 4.0, ecBeats * 60.0 / bpm * sr));

    const int delay = juce::jlimit (0, dryRing.getNumSamples() - 1, dryDelay.load());
    const double latencyBeats = delay * beatsPerSample;

    const float gateCoef = 1.0f - std::exp (-1.0f / (0.002f * (float) sr));
    const float fc = juce::jmin (0.45f * (float) sr, 800.0f * std::pow (25.0f, rvTone));
    const float toneA = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * fc / (float) sr);
    const float fbA = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 5500.0f / (float) sr);

    float* io[2] = { buffer.getWritePointer (0), buffer.getWritePointer (1) };

    // ---- 1) Eingang fuer den Hall: nur an aktiven Feldern ----------------------
    if (wetBuf.getNumSamples() < numSamples)
        wetBuf.setSize (2, numSamples, false, false, true);

    float* wet[2] = { wetBuf.getWritePointer (0), wetBuf.getWritePointer (1) };

    for (int n = 0; n < numSamples; ++n)
    {
        const double pIn = ppq + n * beatsPerSample;
        const float target = (rvOn && rvActive[stepIndexAt (pIn, stepBeats)]) ? 1.0f : 0.0f;
        gateRv += gateCoef * (target - gateRv);
        wet[0][n] = io[0][n] * gateRv;
        wet[1][n] = io[1][n] * gateRv;
    }

    // ---- 2) Faltung mit dem umgedrehten Hall-Schweif -----------------------------
    if (conv.getCurrentIRSize() > 0)
    {
        juce::dsp::AudioBlock<float> all (wetBuf);
        for (int start = 0; start < numSamples; start += maxBlock)
        {
            const int num = juce::jmin (maxBlock, numSamples - start);
            auto sub = all.getSubBlock ((size_t) start, (size_t) num);
            conv.process (juce::dsp::ProcessContextReplacing<float> (sub));
        }
    }
    else
    {
        wetBuf.clear (0, numSamples);
    }

    // ---- 3) Verzoegertes Original + Klang + Tremolo + Echo ---------------------
    float* ring[2]  = { dryRing.getWritePointer (0), dryRing.getWritePointer (1) };
    float* ebuf[2]  = { echoBuf.getWritePointer (0), echoBuf.getWritePointer (1) };
    const int ringSize = dryRing.getNumSamples();

    for (int n = 0; n < numSamples; ++n)
    {
        // Originalsignal durch die Verzoegerung schicken
        ring[0][dryWritePos] = io[0][n];
        ring[1][dryWritePos] = io[1][n];

        int rp = dryWritePos - delay;
        if (rp < 0)
            rp += ringSize;

        const float dryL = ring[0][rp];
        const float dryR = ring[1][rp];

        if (++dryWritePos >= ringSize)
            dryWritePos = 0;

        // Projektposition des Tons, der gerade ausgegeben wird
        const double pSound = ppq + n * beatsPerSample - latencyBeats;

        // Hall: Klang (zwei sanfte Tiefpaesse)
        float wl = wet[0][n];
        float wr = wet[1][n];
        toneLp1[0] += toneA * (wl - toneLp1[0]);  toneLp2[0] += toneA * (toneLp1[0] - toneLp2[0]);
        toneLp1[1] += toneA * (wr - toneLp1[1]);  toneLp2[1] += toneA * (toneLp1[1] - toneLp2[1]);
        wl = toneLp2[0];
        wr = toneLp2[1];

        // Tremolo nur auf dem Hall
        float trGain = 1.0f;
        const float depth = trDepthSm.getNextValue();
        if (depth > 0.0001f)
        {
            double phase = pSound / trBeats;
            phase -= std::floor (phase);
            trGain = 1.0f - depth * 0.5f * (1.0f - std::cos (juce::MathConstants<float>::twoPi * (float) phase));
        }

        const float wetGain = rvOutSm.getNextValue() * rvMix * kWetScale * trGain;
        wl *= wetGain;
        wr *= wetGain;

        // Echo-Wurf: nur Toene an aktiven Feldern gehen ins Echo, danach klingt es aus
        const float eTarget = (ecOn && ecActive[stepIndexAt (pSound, stepBeats)]) ? 1.0f : 0.0f;
        gateEc += gateCoef * (eTarget - gateEc);

        const float dTime = echoDelaySm.getNextValue();
        float rpos = (float) echoWritePos - dTime;
        if (rpos < 0.0f)
            rpos += (float) echoSize;
        const int i0 = juce::jmin ((int) rpos, echoSize - 1);
        const int i1 = (i0 + 1 >= echoSize) ? 0 : i0 + 1;
        const float fr = rpos - (float) i0;

        const float tapL = ebuf[0][i0] + fr * (ebuf[0][i1] - ebuf[0][i0]);
        const float tapR = ebuf[1][i0] + fr * (ebuf[1][i1] - ebuf[1][i0]);

        fbLp[0] += fbA * (tapL - fbLp[0]);
        fbLp[1] += fbA * (tapR - fbLp[1]);

        const float sendL = dryL * gateEc;
        const float sendR = dryR * gateEc;

        float inL, inR;
        if (ping)
        {
            inL = 0.5f * (sendL + sendR) + ecFb * fbLp[1];
            inR = ecFb * fbLp[0];
        }
        else
        {
            inL = sendL + ecFb * fbLp[0];
            inR = sendR + ecFb * fbLp[1];
        }

        ebuf[0][echoWritePos] = std::tanh (inL);
        ebuf[1][echoWritePos] = std::tanh (inR);
        if (++echoWritePos >= echoSize)
            echoWritePos = 0;

        const float ecGain = ecOutSm.getNextValue();

        io[0][n] = dryL + wl + tapL * ecGain;
        io[1][n] = dryR + wr + tapR * ecGain;
    }
}

//==============================================================================
juce::AudioProcessorEditor* FlipVerbAudioProcessor::createEditor()
{
    return new FlipVerbAudioProcessorEditor (*this);
}

void FlipVerbAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void FlipVerbAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FlipVerbAudioProcessor();
}
