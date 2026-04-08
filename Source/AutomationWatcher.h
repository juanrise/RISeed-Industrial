#pragma once

#include <JuceHeader.h>

class AutomationWatcher : public juce::Thread
{
public:
    AutomationWatcher() : juce::Thread("AutomationWatcher") {}
    ~AutomationWatcher() override { stopWatching(); }

    void startWatching(juce::AudioProcessorValueTreeState* newApvts)
    {
        apvts = newApvts;
        targetFile = juce::File::getSpecialLocation(juce::File::userHomeDirectory)
                        .getChildFile("RISeed/automation.json");
        
        lastModTime = targetFile.getLastModificationTime();
        
        if (!isThreadRunning())
            startThread();
    }

    void stopWatching()
    {
        stopThread(1000);
    }

    void run() override
    {
        while (!threadShouldExit())
        {
            if (targetFile.existsAsFile())
            {
                auto modTime = targetFile.getLastModificationTime();
                if (modTime > lastModTime)
                {
                    lastModTime = modTime;
                    parseAndApplyJson();
                }
            }
            wait(200);
        }
    }

private:
    void parseAndApplyJson()
    {
        if (!apvts) return;

        auto jsonParseResult = juce::JSON::parse(targetFile);
        if (jsonParseResult.isObject())
        {
            auto* obj = jsonParseResult.getDynamicObject();
            
            float mixVal = obj->hasProperty("Mix") ? static_cast<float>(obj->getProperty("Mix")) : -1.0f;
            float decayVal = obj->hasProperty("Decay") ? static_cast<float>(obj->getProperty("Decay")) : -1.0f;
            float crunchVal = obj->hasProperty("Crunch") ? static_cast<float>(obj->getProperty("Crunch")) : -1.0f;

            juce::MessageManager::callAsync([this, mixVal, decayVal, crunchVal]()
            {
                if (!apvts) return;
                
                if (mixVal >= 0.0f)
                    if (auto* p = apvts->getParameter("Mix"))
                        p->setValueNotifyingHost(p->convertTo0to1(mixVal));
                
                if (decayVal >= 0.0f)
                    if (auto* p = apvts->getParameter("Decay"))
                        p->setValueNotifyingHost(p->convertTo0to1(decayVal));
                
                if (crunchVal >= 0.0f)
                    if (auto* p = apvts->getParameter("Crunch"))
                        p->setValueNotifyingHost(p->convertTo0to1(crunchVal));
            });
        }
    }

    juce::AudioProcessorValueTreeState* apvts = nullptr;
    juce::File targetFile;
    juce::Time lastModTime;
};
