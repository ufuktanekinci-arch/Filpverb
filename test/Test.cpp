// Offline-Test: schickt kurze Rauschstoesse durch FlipVerb und gibt den Pegelverlauf aus.
// Erwartung (120 BPM, Standardwerte: Hall 2 Schlaege = 1 s, Hall-Felder 1 und 9, Echo-Felder 8 und 16):
//  - Stoss bei 0.0 s liegt in Feld 1 -> Hall baut sich von 0 bis 1 s auf, Original kommt bei 1.0 s
//  - Stoss bei 0.875 s liegt in Feld 8 -> Echo-Wiederholungen nach dem Original (ab ~1.875 s + Abstand)
#include "PluginProcessor.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;

    const double sr = 48000.0;
    const int bs = 512;

    FlipVerbAudioProcessor proc;

    // Optional: "id=wert" (normierter Wert 0..1) setzt einen Parameter, z.B. rvMix=0 oder ecOn=0
    for (int a = 1; a < argc; ++a)
    {
        const juce::String arg (argv[a]);
        const auto id = arg.upToFirstOccurrenceOf ("=", false, false);
        const auto val = arg.fromFirstOccurrenceOf ("=", false, false).getFloatValue();
        if (auto* p = proc.apvts.getParameter (id))
            p->setValueNotifyingHost (val);
        else
            std::printf ("Unbekannter Parameter: %s\n", id.toRawUTF8());
    }

    proc.setPlayConfigDetails (2, 2, sr, bs);
    proc.prepareToPlay (sr, bs);
    juce::Thread::sleep (1500); // der Hall-Schweif wird im Hintergrund geladen

    std::printf ("Latenz: %d Samples (%.3f s)\n", proc.getDryDelaySamples(), proc.getDryDelaySamples() / sr);

    const int total = (int) (6.0 * sr);
    juce::AudioBuffer<float> in (2, total), out (2, total);
    in.clear();
    juce::Random rng (42);

    for (int i = 0; i < total; ++i)
    {
        const double shift = std::getenv ("SHIFT") ? std::atof (std::getenv ("SHIFT")) : 0.0;
        const double t = i / sr - shift;
        const bool burst = (t >= 0.0 && t < 0.1) || (t >= 0.875 && t < 0.975);
        const float v = burst ? 0.5f * (rng.nextFloat() * 2.0f - 1.0f) : 0.0f;
        in.setSample (0, i, v);
        in.setSample (1, i, v);
    }

    juce::MidiBuffer midi;
    int nonFinite = 0;

    for (int pos = 0; pos < total; pos += bs)
    {
        const int n = juce::jmin (bs, total - pos);
        juce::AudioBuffer<float> blk (2, n);
        for (int c = 0; c < 2; ++c)
            blk.copyFrom (c, 0, in, c, pos, n);

        proc.processBlock (blk, midi);

        for (int c = 0; c < 2; ++c)
        {
            out.copyFrom (c, pos, blk, c, 0, n);
            for (int i = 0; i < n; ++i)
                if (! std::isfinite (blk.getSample (c, i)))
                    ++nonFinite;
        }
    }

    if (const char* dump = std::getenv ("DUMP"))
    {
        if (auto* f = std::fopen (dump, "wb"))
        {
            std::fwrite (out.getReadPointer (0), sizeof (float), (size_t) total, f);
            std::fclose (f);
        }
    }

    std::printf ("Nicht-endliche Werte: %d\n", nonFinite);
    std::printf ("Eingang: Stoss bei 0.00-0.10 s und 0.875-0.975 s\n");
    std::printf ("  Zeit(s)  Pegel in dB (RMS, 100-ms-Fenster)   Ausgang\n");

    const int win = (int) (0.1 * sr);
    for (int w = 0; w < 60; ++w)
    {
        double e = 0.0;
        float peak = 0.0f;
        for (int i = 0; i < win; ++i)
        {
            const float s = out.getSample (0, w * win + i);
            e += (double) s * s;
            peak = juce::jmax (peak, std::abs (s));
        }
        const double rms = std::sqrt (e / win);
        const double db = rms > 1e-9 ? 20.0 * std::log10 (rms) : -120.0;
        const int bars = juce::jlimit (0, 60, (int) ((db + 80.0) * 0.75));
        std::printf ("  %5.1f   %7.1f dB   peak %.3f  ", w * 0.1, db, peak);
        for (int b = 0; b < bars; ++b)
            std::printf ("#");
        std::printf ("\n");
    }

    if (const char* snap = std::getenv ("SNAPSHOT"))
    {
        std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        juce::File f (snap);
        f.deleteFile();
        if (auto fo = f.createOutputStream())
            juce::PNGImageFormat().writeImageToStream (img, *fo);
        std::printf ("Screenshot: %s (%dx%d)\n", snap, img.getWidth(), img.getHeight());
    }

    return nonFinite == 0 ? 0 : 1;
}
