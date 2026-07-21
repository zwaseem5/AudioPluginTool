#include "PluginProcessor.h"
#include "PluginEditor.h"

AudioPluginToolEditor::AudioPluginToolEditor (AudioPluginToolProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setLookAndFeel (&lookAndFeel);

    titleLabel.setText ("Audio Plugin Tool", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::FontOptions (24.0f));
    addAndMakeVisible (titleLabel);

    versionLabel.setText ("v0.1.0", juce::dontSendNotification);
    versionLabel.setJustificationType (juce::Justification::centredRight);
    versionLabel.setFont (juce::FontOptions (12.0f));
    versionLabel.setColour (juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible (versionLabel);

    creditLabel.setText ("Created by Waseem Z.", juce::dontSendNotification);
    creditLabel.setJustificationType (juce::Justification::centred);
    creditLabel.setFont (juce::FontOptions (12.0f));
    creditLabel.setColour (juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible (creditLabel);

    addAndMakeVisible (levelGroup);
    addAndMakeVisible (toneGroup);
    addAndMakeVisible (modulationGroup);
    addAndMakeVisible (vocoderGroup);

    setupKnob (gainKnob, "gain", "Gain");
    setupKnob (lowPassKnob, "lowPassCutoff", "Low-Pass");
    setupKnob (highPassKnob, "highPassCutoff", "High-Pass");
    setupKnob (tremoloRateKnob, "tremoloRate", "Rate");
    setupKnob (tremoloDepthKnob, "tremoloDepth", "Depth");
    setupKnob (vocoderMixKnob, "vocoderMix", "Mix");
    setupKnob (vocoderCarrierKnob, "vocoderCarrierHz", "Carrier Pitch");
    setupToggle (vocoderPitchTrackToggle, "vocoderPitchTrack", "Pitch Track");
    setupToggle (vocoderNoteSnapToggle, "vocoderNoteSnap", "Snap to Notes");
    setupCombo (vocoderKeyCombo, "vocoderKey", "Key",
                { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" });
    setupCombo (vocoderScaleCombo, "vocoderScale", "Scale", { "Major", "Minor" });

    setResizable (true, true);
    setResizeLimits (480, 520, 960, 860);
    setSize (640, 660);
}

AudioPluginToolEditor::~AudioPluginToolEditor()
{
    setLookAndFeel (nullptr);
}

void AudioPluginToolEditor::setupKnob (KnobControl& knob, const juce::String& paramID, const juce::String& labelText)
{
    knob.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
    addAndMakeVisible (knob.slider);

    knob.label.setText (labelText, juce::dontSendNotification);
    knob.label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (knob.label);

    knob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.apvts, paramID, knob.slider);
}

void AudioPluginToolEditor::setupToggle (ToggleControl& toggle, const juce::String& paramID, const juce::String& labelText)
{
    addAndMakeVisible (toggle.button);

    toggle.label.setText (labelText, juce::dontSendNotification);
    toggle.label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (toggle.label);

    toggle.attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processorRef.apvts, paramID, toggle.button);
}

void AudioPluginToolEditor::setupCombo (ComboControl& combo, const juce::String& paramID,
                                         const juce::String& labelText, const juce::StringArray& choices)
{
    combo.box.addItemList (choices, 1);
    addAndMakeVisible (combo.box);

    combo.label.setText (labelText, juce::dontSendNotification);
    combo.label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (combo.label);

    combo.attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        processorRef.apvts, paramID, combo.box);
}

void AudioPluginToolEditor::layoutKnobsInRow (juce::Rectangle<int> bounds, const std::vector<KnobControl*>& knobs)
{
    auto width = bounds.getWidth() / (int) knobs.size();

    for (auto* knob : knobs)
    {
        auto knobBounds = bounds.removeFromLeft (width);
        knob->label.setBounds (knobBounds.removeFromTop (20));
        knob->slider.setBounds (knobBounds.reduced (6));
    }
}

void AudioPluginToolEditor::layoutToggleCell (juce::Rectangle<int> bounds, ToggleControl& toggle)
{
    toggle.label.setBounds (bounds.removeFromTop (20));
    toggle.button.setBounds (bounds.withSizeKeepingCentre (24, 24));
}

void AudioPluginToolEditor::layoutComboCell (juce::Rectangle<int> bounds, ComboControl& combo)
{
    combo.label.setBounds (bounds.removeFromTop (20));
    combo.box.setBounds (bounds.reduced (10, 0).withHeight (24));
}

void AudioPluginToolEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void AudioPluginToolEditor::resized()
{
    auto area = getLocalBounds().reduced (20);

    auto titleRow = area.removeFromTop (32);
    versionLabel.setBounds (titleRow.removeFromRight (60));
    titleLabel.setBounds (titleRow);

    area.removeFromTop (10);

    auto footerRow = area.removeFromBottom (16);
    area.removeFromBottom (8);
    creditLabel.setBounds (footerRow);

    // The bottom (Vocoder) row needs extra height for a second internal
    // row of Key/Scale combo boxes, so it isn't simply half the remaining
    // space like the top row is.
    constexpr int comboRowHeight = 50;
    constexpr int comboRowSpacing = 6;
    auto rowHeight = (area.getHeight() - 10 - comboRowSpacing - comboRowHeight) / 2;
    auto topRow = area.removeFromTop (rowHeight);
    area.removeFromTop (10);
    auto bottomRow = area;

    auto totalWidth = topRow.getWidth();
    auto levelWidth = totalWidth / 5;
    auto toneWidth = totalWidth * 2 / 5;

    auto levelArea = topRow.removeFromLeft (levelWidth);
    topRow.removeFromLeft (8);
    auto toneArea = topRow.removeFromLeft (toneWidth);
    topRow.removeFromLeft (8);
    auto modArea = topRow;

    levelGroup.setBounds (levelArea);
    toneGroup.setBounds (toneArea);
    modulationGroup.setBounds (modArea);

    auto inset = [] (juce::Rectangle<int> bounds) { return bounds.reduced (10).withTrimmedTop (20); };

    layoutKnobsInRow (inset (levelArea), { &gainKnob });
    layoutKnobsInRow (inset (toneArea), { &lowPassKnob, &highPassKnob });
    layoutKnobsInRow (inset (modArea), { &tremoloRateKnob, &tremoloDepthKnob });

    auto vocoderArea = bottomRow.withSizeKeepingCentre (totalWidth, bottomRow.getHeight());
    vocoderGroup.setBounds (vocoderArea);

    auto vocoderContent = inset (vocoderArea);
    auto comboRow = vocoderContent.removeFromBottom (comboRowHeight);
    vocoderContent.removeFromBottom (comboRowSpacing);

    auto cellWidth = vocoderContent.getWidth() / 4;
    auto knobsArea = vocoderContent.removeFromLeft (cellWidth * 2);
    auto toggle1Cell = vocoderContent.removeFromLeft (cellWidth);
    auto toggle2Cell = vocoderContent;

    layoutKnobsInRow (knobsArea, { &vocoderMixKnob, &vocoderCarrierKnob });
    layoutToggleCell (toggle1Cell, vocoderPitchTrackToggle);
    layoutToggleCell (toggle2Cell, vocoderNoteSnapToggle);

    auto comboCellWidth = comboRow.getWidth() / 2;
    auto keyCell = comboRow.removeFromLeft (comboCellWidth);
    auto scaleCell = comboRow;

    layoutComboCell (keyCell, vocoderKeyCombo);
    layoutComboCell (scaleCell, vocoderScaleCombo);
}
