#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>
#include "../DSP/Tremolo.h"

class TremoloTests : public juce::UnitTest
{
public:
    TremoloTests() : juce::UnitTest ("Tremolo", "DSP") {}

    void runTest() override
    {
        beginTest ("Depth 0 leaves the signal unchanged");
        {
            Tremolo tremolo;
            juce::dsp::ProcessSpec spec { 44100.0, 512, 1 };
            tremolo.prepare (spec);
            tremolo.setRateHz (5.0f);
            tremolo.setDepth (0.0f);

            juce::AudioBuffer<float> buffer (1, 256);
            for (int i = 0; i < 256; ++i)
                buffer.setSample (0, i, 0.5f);

            juce::dsp::AudioBlock<float> block (buffer);
            juce::dsp::ProcessContextReplacing<float> context (block);
            tremolo.process (context);

            for (int i = 0; i < 256; ++i)
                expectWithinAbsoluteError (buffer.getSample (0, i), 0.5f, 1.0e-6f);
        }

        beginTest ("Depth 1 creates full amplitude modulation");
        {
            Tremolo tremolo;
            juce::dsp::ProcessSpec spec { 44100.0, 512, 1 };
            tremolo.prepare (spec);
            tremolo.setRateHz (5.0f);
            tremolo.setDepth (1.0f);

            const int numSamples = 10000; // more than one LFO cycle at 5 Hz / 44.1 kHz
            juce::AudioBuffer<float> buffer (1, numSamples);
            for (int i = 0; i < numSamples; ++i)
                buffer.setSample (0, i, 1.0f);

            juce::dsp::AudioBlock<float> block (buffer);
            juce::dsp::ProcessContextReplacing<float> context (block);
            tremolo.process (context);

            float minVal = 1.0f, maxVal = 0.0f;
            for (int i = 0; i < numSamples; ++i)
            {
                auto v = std::abs (buffer.getSample (0, i));
                minVal = juce::jmin (minVal, v);
                maxVal = juce::jmax (maxVal, v);
            }

            expect (minVal < 0.05f, "expected the LFO trough to approach silence at depth=1");
            expect (maxVal > 0.95f, "expected the LFO peak to approach full amplitude at depth=1");
        }
    }
};

static TremoloTests tremoloTests;
