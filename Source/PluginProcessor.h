#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/Tremolo.h"
#include "DSP/Clipper.h"
#include "DSP/Vocoder.h"

class AudioPluginToolProcessor : public juce::AudioProcessor
{
public:
    AudioPluginToolProcessor();
    ~AudioPluginToolProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts { *this, nullptr, "PARAMETERS", createParameterLayout() };

private:
    enum ChainIndex
    {
        gainIndex,
        vocoderIndex,
        lowPassIndex,
        highPassIndex,
        tremoloIndex,
        clipperIndex
    };

    using FilterChain = juce::dsp::ProcessorChain<juce::dsp::Gain<float>,
                                                    Vocoder,
                                                    juce::dsp::StateVariableTPTFilter<float>,
                                                    juce::dsp::StateVariableTPTFilter<float>,
                                                    Tremolo,
                                                    Clipper>;
    FilterChain processorChain;

    std::atomic<float>* gainDbParam = nullptr;
    std::atomic<float>* vocoderMixParam = nullptr;
    std::atomic<float>* vocoderCarrierHzParam = nullptr;
    std::atomic<float>* vocoderPitchTrackParam = nullptr;
    std::atomic<float>* vocoderNoteSnapParam = nullptr;
    std::atomic<float>* vocoderKeyParam = nullptr;
    std::atomic<float>* vocoderScaleParam = nullptr;
    std::atomic<float>* lowPassCutoffParam = nullptr;
    std::atomic<float>* highPassCutoffParam = nullptr;
    std::atomic<float>* tremoloRateParam = nullptr;
    std::atomic<float>* tremoloDepthParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginToolProcessor)
};
