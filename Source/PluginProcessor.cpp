#include "PluginProcessor.h"
#include "PluginEditor.h"

AudioPluginToolProcessor::AudioPluginToolProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    gainDbParam = apvts.getRawParameterValue ("gain");
    vocoderMixParam = apvts.getRawParameterValue ("vocoderMix");
    vocoderCarrierHzParam = apvts.getRawParameterValue ("vocoderCarrierHz");
    vocoderPitchTrackParam = apvts.getRawParameterValue ("vocoderPitchTrack");
    vocoderNoteSnapParam = apvts.getRawParameterValue ("vocoderNoteSnap");
    vocoderKeyParam = apvts.getRawParameterValue ("vocoderKey");
    vocoderScaleParam = apvts.getRawParameterValue ("vocoderScale");
    lowPassCutoffParam = apvts.getRawParameterValue ("lowPassCutoff");
    highPassCutoffParam = apvts.getRawParameterValue ("highPassCutoff");
    tremoloRateParam = apvts.getRawParameterValue ("tremoloRate");
    tremoloDepthParam = apvts.getRawParameterValue ("tremoloDepth");

    processorChain.get<lowPassIndex>().setType (juce::dsp::StateVariableTPTFilter<float>::Type::lowpass);
    processorChain.get<highPassIndex>().setType (juce::dsp::StateVariableTPTFilter<float>::Type::highpass);
}

AudioPluginToolProcessor::~AudioPluginToolProcessor()
{
}

void AudioPluginToolProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels = (juce::uint32) getTotalNumOutputChannels();

    processorChain.prepare (spec);
    processorChain.get<gainIndex>().setRampDurationSeconds (0.02);
}

void AudioPluginToolProcessor::releaseResources()
{
}

bool AudioPluginToolProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void AudioPluginToolProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    processorChain.get<gainIndex>().setGainDecibels (gainDbParam->load());
    processorChain.get<vocoderIndex>().setMix (vocoderMixParam->load() / 100.0f);
    processorChain.get<vocoderIndex>().setCarrierFrequency (vocoderCarrierHzParam->load());
    processorChain.get<vocoderIndex>().setPitchTrackingEnabled (vocoderPitchTrackParam->load() > 0.5f);
    processorChain.get<vocoderIndex>().setNoteSnapEnabled (vocoderNoteSnapParam->load() > 0.5f);
    processorChain.get<vocoderIndex>().setMusicalKey ((int) vocoderKeyParam->load(), vocoderScaleParam->load() > 0.5f);
    processorChain.get<lowPassIndex>().setCutoffFrequency (lowPassCutoffParam->load());
    processorChain.get<highPassIndex>().setCutoffFrequency (highPassCutoffParam->load());
    processorChain.get<tremoloIndex>().setRateHz (tremoloRateParam->load());
    processorChain.get<tremoloIndex>().setDepth (tremoloDepthParam->load() / 100.0f);

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);
    processorChain.process (context);
}

juce::AudioProcessorEditor* AudioPluginToolProcessor::createEditor()
{
    return new AudioPluginToolEditor (*this);
}

bool AudioPluginToolProcessor::hasEditor() const
{
    return true;
}

const juce::String AudioPluginToolProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioPluginToolProcessor::acceptsMidi() const
{
    return false;
}

bool AudioPluginToolProcessor::producesMidi() const
{
    return false;
}

bool AudioPluginToolProcessor::isMidiEffect() const
{
    return false;
}

double AudioPluginToolProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioPluginToolProcessor::getNumPrograms()
{
    return 1;
}

int AudioPluginToolProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPluginToolProcessor::setCurrentProgram (int)
{
}

const juce::String AudioPluginToolProcessor::getProgramName (int)
{
    return {};
}

void AudioPluginToolProcessor::changeProgramName (int, const juce::String&)
{
}

void AudioPluginToolProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void AudioPluginToolProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorValueTreeState::ParameterLayout AudioPluginToolProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "gain", 1 },
        "Gain",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "vocoderMix", 1 },
        "Vocoder Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "vocoderCarrierHz", 1 },
        "Vocoder Carrier Pitch",
        juce::NormalisableRange<float> (50.0f, 400.0f, 0.1f),
        110.0f,
        juce::AudioParameterFloatAttributes().withLabel ("Hz")));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "vocoderPitchTrack", 1 },
        "Vocoder Pitch Track",
        true));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "vocoderNoteSnap", 1 },
        "Vocoder Note Snap",
        true));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "vocoderKey", 1 },
        "Vocoder Key",
        juce::StringArray { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" },
        0));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "vocoderScale", 1 },
        "Vocoder Scale",
        juce::StringArray { "Major", "Minor" },
        0));

    juce::NormalisableRange<float> lowPassRange (20.0f, 20000.0f, 1.0f);
    lowPassRange.setSkewForCentre (1000.0f);
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "lowPassCutoff", 1 },
        "Low-Pass Cutoff",
        lowPassRange,
        20000.0f,
        juce::AudioParameterFloatAttributes().withLabel ("Hz")));

    juce::NormalisableRange<float> highPassRange (20.0f, 20000.0f, 1.0f);
    highPassRange.setSkewForCentre (1000.0f);
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "highPassCutoff", 1 },
        "High-Pass Cutoff",
        highPassRange,
        20.0f,
        juce::AudioParameterFloatAttributes().withLabel ("Hz")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "tremoloRate", 1 },
        "Tremolo Rate",
        juce::NormalisableRange<float> (0.1f, 20.0f, 0.01f),
        5.0f,
        juce::AudioParameterFloatAttributes().withLabel ("Hz")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "tremoloDepth", 1 },
        "Tremolo Depth",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    return { params.begin(), params.end() };
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginToolProcessor();
}
