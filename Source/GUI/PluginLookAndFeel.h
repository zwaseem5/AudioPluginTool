#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// Dark theme + custom flat-style rotary knob (arc indicator instead of the
// default JUCE "pointer on a circle" look) so the plugin doesn't look like
// an unstyled default JUCE app.
class PluginLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PluginLookAndFeel();

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                            float sliderPosProportional, float rotaryStartAngle,
                            float rotaryEndAngle, juce::Slider& slider) override;
};
