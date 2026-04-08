#pragma once

#include <JuceHeader.h>

class AutomationWatcher : public juce::Thread
{
public:
    static AutomationWatcher* getInstance()
    {
        static AutomationWatcher instance;
        return &instance;
    }

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
    AutomationWatcher() : juce::Thread("AutomationWatcher") {}
    ~AutomationWatcher() override { stopWatching(); }

    void parseAndApplyJson()
    {
        if (!apvts) return;

        auto jsonParseResult = juce::JSON::parse(targetFile);
        if (jsonParseResult.isObject())
        {
            auto* obj = jsonParseResult.getDynamicObject();
            if (obj->hasProperty("Mix")) 
                if (auto* p = apvts->getParameter("Mix")) 
                    p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(obj->getProperty("Mix"))));
            
            if (obj->hasProperty("Decay")) 
                if (auto* p = apvts->getParameter("Decay")) 
                    p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(obj->getProperty("Decay"))));

            if (obj->hasProperty("Crunch")) 
                if (auto* p = apvts->getParameter("Crunch")) 
                    p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(obj->getProperty("Crunch"))));
        }
    }

    juce::AudioProcessorValueTreeState* apvts = nullptr;
    juce::File targetFile;
    juce::Time lastModTime;
};
