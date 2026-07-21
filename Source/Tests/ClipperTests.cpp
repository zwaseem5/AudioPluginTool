#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>
#include "../DSP/Clipper.h"

class ClipperTests : public juce::UnitTest
{
public:
    ClipperTests() : juce::UnitTest ("Clipper", "DSP") {}

    void runTest() override
    {
        beginTest ("Output is always bounded within [-1, 1], even for extreme input");
        {
            Clipper clipper;
            juce::dsp::ProcessSpec spec { 44100.0, 512, 1 };
            clipper.prepare (spec);

            const float extremeValues[] = { -1000.0f, -1.0f, 0.0f, 1.0f, 1000.0f };
            juce::AudioBuffer<float> buffer (1, 5);
            for (int i = 0; i < 5; ++i)
                buffer.setSample (0, i, extremeValues[i]);

            juce::dsp::AudioBlock<float> block (buffer);
            juce::dsp::ProcessContextReplacing<float> context (block);
            clipper.process (context);

            for (int i = 0; i < 5; ++i)
            {
                auto v = buffer.getSample (0, i);
                expect (v >= -1.0f && v <= 1.0f, "sample out of bounds: " + juce::String (v));
            }
        }

        beginTest ("Silence in produces silence out");
        {
            Clipper clipper;
            juce::dsp::ProcessSpec spec { 44100.0, 512, 1 };
            clipper.prepare (spec);

            juce::AudioBuffer<float> buffer (1, 100);
            buffer.clear();

            juce::dsp::AudioBlock<float> block (buffer);
            juce::dsp::ProcessContextReplacing<float> context (block);
            clipper.process (context);

            for (int i = 0; i < 100; ++i)
                expectWithinAbsoluteError (buffer.getSample (0, i), 0.0f, 1.0e-6f);
        }
    }
};

static ClipperTests clipperTests;
