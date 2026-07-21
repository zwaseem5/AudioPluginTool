#pragma once

#include <juce_dsp/juce_dsp.h>
#include <vector>

// Simple autocorrelation pitch detector for a monophonic voice signal.
// Autocorrelation works by sliding a copy of a signal against itself by an
// increasing time offset ("lag") and measuring how well they line up at
// each offset; the lag with the strongest match corresponds to the
// waveform's fundamental period, so frequency = sampleRate / bestLag.
// It's a classic, simple pitch-detection technique -- not as accurate as
// the algorithms used in professional pitch correction plugins, but more
// than adequate for driving a vocoder's carrier pitch from a sung note.
class PitchTracker
{
public:
    void prepare (double sampleRateIn)
    {
        sampleRate = sampleRateIn;

        // 30ms analysis window: long enough to contain at least two full
        // cycles of the lowest note we want to detect (~70 Hz, period
        // ~14ms), short enough to feel responsive when singing.
        windowSize = (int) (sampleRate * 0.03);
        buffer.assign ((size_t) windowSize, 0.0f);
        writePos = 0;
        smoothedFrequency = 110.0f;
    }

    void reset()
    {
        std::fill (buffer.begin(), buffer.end(), 0.0f);
        writePos = 0;
    }

    // Feed one block's worth of (mono) input samples, then re-estimate pitch.
    void pushSamples (const float* samples, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            buffer[(size_t) writePos] = samples[i];
            writePos = (writePos + 1) % windowSize;
        }

        detectPitch();
    }

    float getFrequencyHz() const { return smoothedFrequency; }

private:
    void detectPitch()
    {
        float energy = 0.0f;
        for (auto s : buffer)
            energy += s * s;
        energy /= (float) buffer.size();

        // Too quiet to reliably detect a pitch (silence/breath noise) --
        // hold the last known frequency rather than lock onto noise.
        if (energy < 1.0e-5f)
            return;

        auto minLag = (int) (sampleRate / maxDetectableHz);
        auto maxLag = juce::jmin ((int) (sampleRate / minDetectableHz), windowSize - 1);

        float bestCorrelation = 0.0f;
        int bestLag = -1;

        for (int lag = minLag; lag <= maxLag; ++lag)
        {
            float sum = 0.0f;
            for (int i = 0; i < windowSize - lag; ++i)
            {
                auto a = buffer[(size_t) ((writePos + i) % windowSize)];
                auto b = buffer[(size_t) ((writePos + i + lag) % windowSize)];
                sum += a * b;
            }

            if (sum > bestCorrelation)
            {
                bestCorrelation = sum;
                bestLag = lag;
            }
        }

        if (bestLag > 0)
        {
            auto detectedHz = (float) sampleRate / (float) bestLag;

            // Slew-limit rather than jump instantly: smooths over octave
            // errors and frame-to-frame jitter that a simple autocorrelator
            // is prone to, at the cost of a small amount of lag when a
            // sung note first changes.
            smoothedFrequency += (detectedHz - smoothedFrequency) * 0.25f;
        }
    }

    static constexpr float minDetectableHz = 70.0f;
    static constexpr float maxDetectableHz = 500.0f;

    double sampleRate = 44100.0;
    int windowSize = 1323;
    std::vector<float> buffer;
    int writePos = 0;
    float smoothedFrequency = 110.0f;
};
