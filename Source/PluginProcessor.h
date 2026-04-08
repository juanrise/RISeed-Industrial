#pragma once

#include <JuceHeader.h>
#include "DSP/ReverbController.h"
#include "AutomationWatcher.h"

class RISeedIndustrialProcessor  : public juce::AudioProcessor
{
public:
    RISeedIndustrialProcessor();
    ~RISeedIndustrialProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 60.0; } // Max reverb tail

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int index) override {}
    const juce::String getProgramName (int index) override { return {}; }
    void changeProgramName (int index, const juce::String& newName) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    std::unique_ptr<Cloudseed::ReverbController> reverb;
    
    juce::dsp::Oversampling<float> oversampler;
    juce::SmoothedValue<float> smoothedCrunch;
    std::vector<float> crunchScratch; // Pre-allocated per-sample crunch values (no audio-thread alloc)
    
    AutomationWatcher automationWatcher;

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RISeedIndustrialProcessor)
};
