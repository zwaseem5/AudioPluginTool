#include <juce_core/juce_core.h>
#include "../PluginProcessor.h"

class DSPChainTests : public juce::UnitTest
{
public:
    DSPChainTests() : juce::UnitTest ("DSP Chain", "Plugin") {}

    void runTest() override
    {
        beginTest ("Stereo bus layout is accepted");
        {
            AudioPluginToolProcessor processor;
            juce::AudioProcessor::BusesLayout layout;
            layout.inputBuses.add (juce::AudioChannelSet::stereo());
            layout.outputBuses.add (juce::AudioChannelSet::stereo());
            expect (processor.isBusesLayoutSupported (layout));
        }

        beginTest ("Mismatched input/output channel counts are rejected");
        {
            AudioPluginToolProcessor processor;
            juce::AudioProcessor::BusesLayout layout;
            layout.inputBuses.add (juce::AudioChannelSet::mono());
            layout.outputBuses.add (juce::AudioChannelSet::stereo());
            expect (! processor.isBusesLayoutSupported (layout));
        }

        beginTest ("processBlock survives an empty (zero-sample) buffer");
        {
            AudioPluginToolProcessor processor;
            processor.prepareToPlay (44100.0, 512);

            juce::AudioBuffer<float> buffer (2, 0);
            juce::MidiBuffer midi;
            processor.processBlock (buffer, midi);

            expect (true); // reaching this line without crashing/asserting is the test
        }

        beginTest ("Processor survives a sample-rate change without crashing");
        {
            AudioPluginToolProcessor processor;
            processor.prepareToPlay (44100.0, 512);
            processor.prepareToPlay (48000.0, 256);

            juce::AudioBuffer<float> buffer (2, 256);
            buffer.clear();
            juce::MidiBuffer midi;
            processor.processBlock (buffer, midi);

            expect (true);
        }
    }
};

static DSPChainTests dspChainTests;
