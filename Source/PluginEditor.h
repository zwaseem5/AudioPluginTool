#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include "PluginProcessor.h"
#include "GUI/PluginLookAndFeel.h"

class AudioPluginToolEditor : public juce::AudioProcessorEditor
{
public:
    explicit AudioPluginToolEditor (AudioPluginToolProcessor&);
    ~AudioPluginToolEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct KnobControl
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    struct ToggleControl
    {
        juce::ToggleButton button;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
    };

    struct ComboControl
    {
        juce::ComboBox box;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
    };

    void setupKnob (KnobControl& knob, const juce::String& paramID, const juce::String& labelText);
    void setupToggle (ToggleControl& toggle, const juce::String& paramID, const juce::String& labelText);
    void setupCombo (ComboControl& combo, const juce::String& paramID, const juce::String& labelText,
                      const juce::StringArray& choices);
    static void layoutKnobsInRow (juce::Rectangle<int> bounds, const std::vector<KnobControl*>& knobs);
    static void layoutToggleCell (juce::Rectangle<int> bounds, ToggleControl& toggle);
    static void layoutComboCell (juce::Rectangle<int> bounds, ComboControl& combo);

    AudioPluginToolProcessor& processorRef;

    PluginLookAndFeel lookAndFeel;

    juce::Label titleLabel;
    juce::Label versionLabel;
    juce::Label creditLabel;

    juce::GroupComponent levelGroup { {}, "Level" };
    juce::GroupComponent toneGroup { {}, "Tone" };
    juce::GroupComponent modulationGroup { {}, "Modulation" };
    juce::GroupComponent vocoderGroup { {}, "Vocoder" };

    KnobControl gainKnob, lowPassKnob, highPassKnob, tremoloRateKnob, tremoloDepthKnob;
    KnobControl vocoderMixKnob, vocoderCarrierKnob;
    ToggleControl vocoderPitchTrackToggle, vocoderNoteSnapToggle;
    ComboControl vocoderKeyCombo, vocoderScaleCombo;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginToolEditor)
};
