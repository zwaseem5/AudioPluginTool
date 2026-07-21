#include "PluginLookAndFeel.h"

namespace
{
    const juce::Colour backgroundColour { 0xff1c1c22 };
    const juce::Colour panelOutlineColour { 0xff3a3a44 };
    const juce::Colour accentColour { 0xff4fd1c5 };
    const juce::Colour textColour { 0xffe4e4e8 };
    const juce::Colour knobBodyColour { 0xff2a2a32 };
}

PluginLookAndFeel::PluginLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, backgroundColour);

    setColour (juce::Slider::rotarySliderFillColourId, accentColour);
    setColour (juce::Slider::rotarySliderOutlineColourId, panelOutlineColour);
    setColour (juce::Slider::thumbColourId, accentColour);
    setColour (juce::Slider::textBoxTextColourId, textColour);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);

    setColour (juce::Label::textColourId, textColour);

    setColour (juce::GroupComponent::outlineColourId, panelOutlineColour);
    setColour (juce::GroupComponent::textColourId, accentColour);
}

void PluginLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPos, float rotaryStartAngle,
                                           float rotaryEndAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (6.0f);
    auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto centre = bounds.getCentre();
    auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    auto lineWidth = 4.0f;
    auto arcRadius = radius - lineWidth * 0.5f;

    juce::Path backgroundArc;
    backgroundArc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                                  rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (slider.findColour (juce::Slider::rotarySliderOutlineColourId));
    g.strokePath (backgroundArc, juce::PathStrokeType (lineWidth, juce::PathStrokeType::curved,
                                                         juce::PathStrokeType::rounded));

    juce::Path valueArc;
    valueArc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                             rotaryStartAngle, toAngle, true);
    g.setColour (slider.findColour (juce::Slider::rotarySliderFillColourId));
    g.strokePath (valueArc, juce::PathStrokeType (lineWidth, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));

    auto knobRadius = arcRadius - lineWidth;
    g.setColour (knobBodyColour);
    g.fillEllipse (centre.x - knobRadius, centre.y - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f);
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawEllipse (centre.x - knobRadius, centre.y - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f, 1.0f);

    juce::Path pointer;
    auto pointerLength = knobRadius * 0.75f;
    auto pointerThickness = 3.0f;
    pointer.addRoundedRectangle (-pointerThickness * 0.5f, -knobRadius, pointerThickness, pointerLength, 1.5f);
    pointer.applyTransform (juce::AffineTransform::rotation (toAngle).translated (centre.x, centre.y));
    g.setColour (slider.findColour (juce::Slider::rotarySliderFillColourId));
    g.fillPath (pointer);
}
