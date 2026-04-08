#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "AutomationWatcher.h"

RISeedIndustrialProcessor::RISeedIndustrialProcessor()
    : AudioProcessor (BusesProperties()
                     .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                     .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout()),
      oversampler(2, 2, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true)
{
    reverb = std::make_unique<Cloudseed::ReverbController>(48000);
    smoothedCrunch.reset(48000, 0.05); // 50ms smooth
}

RISeedIndustrialProcessor::~RISeedIndustrialProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout RISeedIndustrialProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>("Mix", "Mix", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("Decay", "Decay", 0.1f, 10.0f, 2.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("Crunch", "Crunch", 1.0f, 20.0f, 1.0f));

    return { params.begin(), params.end() };
}

void RISeedIndustrialProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    reverb->SetSamplerate(sampleRate);
    reverb->ClearBuffers();

    smoothedCrunch.reset(sampleRate, 0.05);
    
    // Set 4x processing (factor 2 => 2^2 = 4x)
    oversampler.initProcessing(samplesPerBlock);

    // Initialize all basic CloudSeed parameters to a default "Industrial" state
    reverb->SetParameter(Cloudseed::Parameter::DryOut, -100.0); // We do our own mixing!
    reverb->SetParameter(Cloudseed::Parameter::EarlyOut, 0.0);
    reverb->SetParameter(Cloudseed::Parameter::LateOut, 0.0); // We'll set this dynamically
    reverb->SetParameter(Cloudseed::Parameter::LateLineCount, 12.0);
    
    // We will start FileWatcher and other singletons elsewhere if needed
    AutomationWatcher::getInstance()->startWatching(&apvts);
}

void RISeedIndustrialProcessor::releaseResources()
{
    oversampler.reset();
}

void RISeedIndustrialProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Update Params
    float mix = apvts.getRawParameterValue("Mix")->load();
    float decay = apvts.getRawParameterValue("Decay")->load();
    float targetCrunch = apvts.getRawParameterValue("Crunch")->load();
    
    smoothedCrunch.setTargetValue(targetCrunch);
    
    reverb->SetParameter(Cloudseed::Parameter::LateLineDecay, decay);
    // Let's ensure out is strictly 0dB FS
    reverb->SetParameter(Cloudseed::Parameter::LateOut, 0.0); 

    int numSamples = buffer.getNumSamples();
    
    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer, true); // backup dry

    // 1) Reverb on wet buffer
    float* channelDataL = buffer.getWritePointer(0);
    float* channelDataR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;
    
    if (channelDataR != nullptr)
        reverb->Process(channelDataL, channelDataR, channelDataL, channelDataR, numSamples);
    else
        reverb->Process(channelDataL, channelDataL, channelDataL, channelDataL, numSamples); // Mono

    // 2) Saturate wet only
    juce::dsp::AudioBlock<float> wetBlock(buffer);
    juce::dsp::AudioBlock<float> osBlock = oversampler.processSamplesUp(wetBlock);

    for (size_t ch = 0; ch < osBlock.getNumChannels(); ++ch)
    {
        float* data = osBlock.getChannelPointer(ch);
        for (int i = 0; i < (int)osBlock.getNumSamples(); ++i)
        {
            float crunch = smoothedCrunch.getNextValue();
            
            // Crunch
            data[i] = std::tanh(crunch * data[i]);
            // Normalize back to prevent large volume jumps
            data[i] *= (1.0f / std::sqrt(std::max(1.0f, crunch))); 
        }
    }
    
    oversampler.processSamplesDown(wetBlock);

    // 3) Dry/Wet Merge
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* writePtr = buffer.getWritePointer(channel);
        auto* dryPtr = dryBuffer.getReadPointer(channel);
        
        for (int i = 0; i < numSamples; ++i)
        {
            writePtr[i] = (dryPtr[i] * (1.0f - mix)) + (writePtr[i] * mix);
        }
    }
}

void RISeedIndustrialProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void RISeedIndustrialProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessorEditor* RISeedIndustrialProcessor::createEditor()
{
    return new RISeedIndustrialEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RISeedIndustrialProcessor();
}
