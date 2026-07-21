#pragma once

#include <juce_dsp/juce_dsp.h>

// Amplitude modulation effect: multiplies the signal by a slowly oscillating
// value derived from a sine LFO. Depth 0 = no effect, depth 1 = LFO swings
// all the way down to silence at its minima.
class Tremolo
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        lfo.prepare (spec);
        lfo.initialise ([] (float x) { return std::sin (x); });
    }

    void reset()
    {
        lfo.reset();
    }

    void setRateHz (float newRateHz)
    {
        lfo.setFrequency (newRateHz);
    }

    void setDepth (float newDepth)
    {
        depth = juce::jlimit (0.0f, 1.0f, newDepth);
    }

    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
        const auto& inputBlock = context.getInputBlock();
        auto& outputBlock = context.getOutputBlock();
        const auto numChannels = outputBlock.getNumChannels();
        const auto numSamples = outputBlock.getNumSamples();

        if (context.isBypassed)
        {
            outputBlock.copyFrom (inputBlock);
            return;
        }

        for (size_t sample = 0; sample < numSamples; ++sample)
        {
            auto lfoValue = lfo.processSample (0.0f);
            auto modulation = 1.0f - depth + depth * (0.5f * (1.0f + lfoValue));

            for (size_t channel = 0; channel < numChannels; ++channel)
                outputBlock.getChannelPointer (channel)[sample] =
                    inputBlock.getChannelPointer (channel)[sample] * modulation;
        }
    }

private:
    juce::dsp::Oscillator<float> lfo;
    float depth = 0.0f;
};
