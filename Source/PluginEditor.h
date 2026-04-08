#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class RISeedIndustrialEditor  : public juce::AudioProcessorEditor
{
public:
    RISeedIndustrialEditor (RISeedIndustrialProcessor&);
    ~RISeedIndustrialEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    RISeedIndustrialProcessor& audioProcessor;

    class CustomLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        CustomLookAndFeel()
        {
            setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff32cd32));
            setColour(juce::Slider::thumbColourId, juce::Colours::white);
        }

        void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                              float sliderPosProportional, float rotaryStartAngle,
                              float rotaryEndAngle, juce::Slider& slider) override
        {
            auto radius = (float) juce::jmin (width / 2, height / 2) - 4.0f;
            auto centreX = (float) x + (float) width  * 0.5f;
            auto centreY = (float) y + (float) height * 0.5f;
            auto rx = centreX - radius;
            auto ry = centreY - radius;
            auto rw = radius * 2.0f;
            auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

            // fill
            g.setColour (juce::Colour(0xff1a1a1a));
            g.fillEllipse (rx, ry, rw, rw);

            // outline
            g.setColour (juce::Colour(0xff32cd32));
            g.drawEllipse (rx, ry, rw, rw, 2.0f);

            juce::Path p;
            auto pointerLength = radius * 0.8f;
            auto pointerThickness = 3.0f;
            p.addRectangle (-pointerThickness * 0.5f, -radius, pointerThickness, pointerLength);
            p.applyTransform (juce::AffineTransform::rotation (angle).translated (centreX, centreY));

            g.setColour (juce::Colour(0xff32cd32));
            g.fillPath (p);
        }
    } customLookAndFeel;

    juce::Slider mixSlider;
    juce::Slider decaySlider;
    juce::Slider crunchSlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> decayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> crunchAttachment;

    struct PresetInfo 
    {
        juce::String name;
        float mix;
        float decay;
        float crunch;
    };

    struct PresetCategory
    {
        juce::String title;
        std::vector<PresetInfo> presets;
    };

    std::vector<PresetCategory> presetMatrix;

    void loadPreset(const PresetInfo& info);
    void saveUserPreset(int slotIdx);
    
    // Simple rect bounds for clicks
    std::vector<std::pair<juce::Rectangle<int>, std::function<void(const juce::MouseEvent&)>>> clickZones;

    void mouseDown(const juce::MouseEvent& e) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RISeedIndustrialEditor)
};
