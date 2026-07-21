#include <juce_core/juce_core.h>
#include "../PluginProcessor.h"

class ParameterLayoutTests : public juce::UnitTest
{
public:
    ParameterLayoutTests() : juce::UnitTest ("Parameter Layout", "Plugin") {}

    void runTest() override
    {
        AudioPluginToolProcessor processor;

        beginTest ("All expected parameters exist");
        {
            const char* expectedIds[] = { "gain", "lowPassCutoff", "highPassCutoff",
                                           "tremoloRate", "tremoloDepth" };
            for (auto* id : expectedIds)
                expect (processor.apvts.getParameter (id) != nullptr,
                        juce::String ("missing parameter: ") + id);
        }

        beginTest ("Default values are transparent (no audible change out of the box)");
        {
            expectWithinAbsoluteError (processor.apvts.getRawParameterValue ("gain")->load(), 0.0f, 1.0e-6f);
            expectWithinAbsoluteError (processor.apvts.getRawParameterValue ("lowPassCutoff")->load(), 20000.0f, 1.0e-3f);
            expectWithinAbsoluteError (processor.apvts.getRawParameterValue ("highPassCutoff")->load(), 20.0f, 1.0e-3f);
            expectWithinAbsoluteError (processor.apvts.getRawParameterValue ("tremoloDepth")->load(), 0.0f, 1.0e-6f);
        }

        beginTest ("Gain range matches spec (-24 to +24 dB)");
        {
            auto* gainParam = dynamic_cast<juce::AudioParameterFloat*> (processor.apvts.getParameter ("gain"));
            expect (gainParam != nullptr);

            if (gainParam != nullptr)
            {
                expectWithinAbsoluteError (gainParam->range.start, -24.0f, 1.0e-3f);
                expectWithinAbsoluteError (gainParam->range.end, 24.0f, 1.0e-3f);
            }
        }
    }
};

static ParameterLayoutTests parameterLayoutTests;
