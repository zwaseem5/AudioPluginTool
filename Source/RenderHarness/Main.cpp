// Command-line tool used by the Python test scripts (see /tests). It loads
// the plugin's AudioProcessor directly -- no host, no GUI -- feeds it a
// known test signal, and writes the processed result to a WAV file so
// Python can analyse it (FFT for filter response, peak checks for
// clipping, impulse position for latency, etc).

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <iostream>
#include "../PluginProcessor.h"

using namespace juce;

namespace
{
    String getArg (const StringArray& args, const String& name, const String& defaultValue)
    {
        auto prefix = "--" + name + "=";

        for (auto& arg : args)
            if (arg.startsWith (prefix))
                return arg.substring (prefix.length());

        return defaultValue;
    }
}

int main (int argc, char* argv[])
{
    ScopedJuceInitialiser_GUI juceInit;

    StringArray args;
    for (int i = 1; i < argc; ++i)
        args.add (argv[i]);

    auto signalType   = getArg (args, "signal", "noise");
    auto outputPath   = getArg (args, "output", "");
    auto sampleRate   = getArg (args, "samplerate", "44100").getDoubleValue();
    auto duration     = getArg (args, "duration", "2.0").getDoubleValue();
    auto seed         = getArg (args, "seed", "42").getIntValue();
    auto gainDb       = getArg (args, "gain", "0.0").getFloatValue();
    auto lowPassHz    = getArg (args, "lowpass", "20000").getFloatValue();
    auto highPassHz   = getArg (args, "highpass", "20").getFloatValue();
    auto tremRateHz   = getArg (args, "tremolo-rate", "5.0").getFloatValue();
    auto tremDepthPct = getArg (args, "tremolo-depth", "0.0").getFloatValue();
    auto amplitude    = getArg (args, "amplitude", "1.0").getFloatValue();
    auto vocoderMixPct = getArg (args, "vocoder-mix", "0.0").getFloatValue();
    auto vocoderCarrierHz = getArg (args, "vocoder-carrier-hz", "110.0").getFloatValue();
    auto vocoderPitchTrack = getArg (args, "vocoder-pitch-track", "1").getFloatValue();
    auto vocoderNoteSnap = getArg (args, "vocoder-note-snap", "1").getFloatValue();
    auto vocoderKey = getArg (args, "vocoder-key", "0").getFloatValue();
    auto vocoderScale = getArg (args, "vocoder-scale", "0").getFloatValue();
    auto toneHz = getArg (args, "tone-hz", "220.0").getFloatValue();

    if (outputPath.isEmpty())
    {
        std::cerr << "Usage: RenderHarness --output=path.wav [--signal=noise|impulse|bursts|tone] "
                     "[--gain=dB] [--lowpass=Hz] [--highpass=Hz] "
                     "[--tremolo-rate=Hz] [--tremolo-depth=percent] [--amplitude=0-1] "
                     "[--vocoder-mix=percent] [--vocoder-carrier-hz=Hz] [--vocoder-pitch-track=0|1] "
                     "[--vocoder-note-snap=0|1] [--vocoder-key=0-11] [--vocoder-scale=0|1] "
                     "[--tone-hz=Hz] [--duration=seconds] [--samplerate=Hz] [--seed=int]\n";
        return 1;
    }

    const int numChannels = 2;
    const int numSamples = (int) (sampleRate * duration);

    AudioBuffer<float> buffer (numChannels, numSamples);
    buffer.clear();

    if (signalType == "impulse")
    {
        for (int ch = 0; ch < numChannels; ++ch)
            buffer.setSample (ch, 0, amplitude);
    }
    else if (signalType == "bursts")
    {
        // Alternating noise/silence, mimicking the on/off rhythm of spoken
        // syllables -- used to verify an effect actually tracks the input's
        // dynamics over time, rather than producing a constant output
        // regardless of what's being said.
        Random rng (seed);
        const int cycleSamples = (int) (0.2 * sampleRate); // 200ms on, 200ms off
        const int onSamples = cycleSamples / 2;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            for (int i = 0; i < numSamples; ++i)
                data[i] = (i % cycleSamples) < onSamples
                              ? amplitude * (rng.nextFloat() * 2.0f - 1.0f)
                              : 0.0f;
        }
    }
    else if (signalType == "tone")
    {
        // A clean sine at a known frequency -- stands in for "singing a
        // single sustained note" when testing pitch-tracking behavior.
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            for (int i = 0; i < numSamples; ++i)
                data[i] = amplitude * std::sin (juce::MathConstants<float>::twoPi * toneHz * (float) i / (float) sampleRate);
        }
    }
    else
    {
        Random rng (seed);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            for (int i = 0; i < numSamples; ++i)
                data[i] = amplitude * (rng.nextFloat() * 2.0f - 1.0f);
        }
    }

    AudioPluginToolProcessor processor;
    processor.apvts.getRawParameterValue ("gain")->store (gainDb);
    processor.apvts.getRawParameterValue ("lowPassCutoff")->store (lowPassHz);
    processor.apvts.getRawParameterValue ("highPassCutoff")->store (highPassHz);
    processor.apvts.getRawParameterValue ("tremoloRate")->store (tremRateHz);
    processor.apvts.getRawParameterValue ("tremoloDepth")->store (tremDepthPct);
    processor.apvts.getRawParameterValue ("vocoderMix")->store (vocoderMixPct);
    processor.apvts.getRawParameterValue ("vocoderCarrierHz")->store (vocoderCarrierHz);
    processor.apvts.getRawParameterValue ("vocoderPitchTrack")->store (vocoderPitchTrack);
    processor.apvts.getRawParameterValue ("vocoderNoteSnap")->store (vocoderNoteSnap);
    processor.apvts.getRawParameterValue ("vocoderKey")->store (vocoderKey);
    processor.apvts.getRawParameterValue ("vocoderScale")->store (vocoderScale);

    const int blockSize = 512;
    processor.prepareToPlay (sampleRate, blockSize);

    MidiBuffer midi;
    for (int start = 0; start < numSamples; start += blockSize)
    {
        auto thisBlock = jmin (blockSize, numSamples - start);
        AudioBuffer<float> blockView (buffer.getArrayOfWritePointers(), numChannels, start, thisBlock);
        processor.processBlock (blockView, midi);
    }

    processor.releaseResources();

    File outFile (outputPath);
    outFile.deleteFile();
    outFile.getParentDirectory().createDirectory();

    std::unique_ptr<OutputStream> outStream (outFile.createOutputStream());
    if (outStream == nullptr)
    {
        std::cerr << "Failed to open output file: " << outputPath << "\n";
        return 1;
    }

    WavAudioFormat wavFormat;
    auto writerOptions = AudioFormatWriterOptions()
                              .withSampleRate (sampleRate)
                              .withNumChannels (numChannels)
                              .withBitsPerSample (32);

    std::unique_ptr<AudioFormatWriter> writer (wavFormat.createWriterFor (outStream, writerOptions));

    if (writer == nullptr)
    {
        std::cerr << "Failed to create WAV writer\n";
        return 1;
    }

    writer->writeFromAudioSampleBuffer (buffer, 0, numSamples);

    return 0;
}
