#pragma once

#include <juce_dsp/juce_dsp.h>

// Soft-clipping safety stage. As the signal approaches 0 dBFS, tanh()
// gracefully saturates it instead of hard-clipping (chopping samples flat
// at +-1.0), which is what causes harsh, crackling digital distortion.
// tanh() is bounded to (-1, 1) for any input, so no matter how much gain
// or tremolo boost happens upstream, the output can never exceed 0 dBFS.
class Clipper
{
public:
    void prepare (const juce::dsp::ProcessSpec&) {}
    void reset() {}

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

        for (size_t channel = 0; channel < numChannels; ++channel)
        {
            auto* in = inputBlock.getChannelPointer (channel);
            auto* out = outputBlock.getChannelPointer (channel);

            for (size_t sample = 0; sample < numSamples; ++sample)
                out[sample] = std::tanh (in[sample]);
        }
    }
};
