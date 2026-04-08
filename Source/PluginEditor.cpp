#include "PluginProcessor.h"
#include "PluginEditor.h"

RISeedIndustrialEditor::RISeedIndustrialEditor (RISeedIndustrialProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (800, 450);
    juce::LookAndFeel::setDefaultLookAndFeel(&customLookAndFeel);

    auto setupSlider = [this](juce::Slider& s, const juce::String& name) {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        addAndMakeVisible(s);
    };

    setupSlider(mixSlider, "Mix");
    setupSlider(decaySlider, "Decay");
    setupSlider(crunchSlider, "Crunch");

    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "Mix", mixSlider);
    decayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "Decay", decaySlider);
    crunchAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "Crunch", crunchSlider);

    // Initialize Preset Matrix
    presetMatrix = {
        { "Small Rooms", { {"Box", 0.3f, 0.5f, 1.2f}, {"Studio", 0.4f, 1.0f, 1.0f}, {"Chamber", 0.5f, 1.5f, 1.5f} } },
        { "Large Halls", { {"Concert", 0.6f, 4.0f, 1.1f}, {"Cathedral", 0.7f, 6.0f, 2.0f}, {"Arena", 0.8f, 8.0f, 3.0f} } },
        { "Shimmer", { {"Angel", 0.6f, 5.0f, 1.0f}, {"Ascend", 0.7f, 7.0f, 1.0f}, {"Ethereal", 0.8f, 9.0f, 1.0f} } },
        { "Industrial", { {"Rust", 0.5f, 2.0f, 8.0f}, {"Grinder", 0.6f, 3.0f, 15.0f}, {"Oblivion", 1.0f, 10.0f, 20.0f} } },
        { "User", { {"User 1", 0.5f, 2.0f, 1.0f}, {"User 2", 0.5f, 2.0f, 1.0f}, {"User 3", 0.5f, 2.0f, 1.0f} } }
    };

    // Attempt to load User Presets
    juce::File userFile = juce::File::getSpecialLocation(juce::File::userHomeDirectory).getChildFile("RISeed/user_presets.json");
    if (userFile.existsAsFile())
    {
        auto parsed = juce::JSON::parse(userFile);
        if (parsed.isArray())
        {
            auto* arr = parsed.getArray();
            for (int i = 0; i < juce::jmin(3, arr->size()); ++i)
            {
                auto obj = arr->getReference(i).getDynamicObject();
                if (obj)
                {
                    presetMatrix[4].presets[i].mix = obj->getProperty("Mix");
                    presetMatrix[4].presets[i].decay = obj->getProperty("Decay");
                    presetMatrix[4].presets[i].crunch = obj->getProperty("Crunch");
                }
            }
        }
    }
}

RISeedIndustrialEditor::~RISeedIndustrialEditor()
{
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
}

void RISeedIndustrialEditor::paint (juce::Graphics& g)
{
    // Background
    g.fillAll (juce::Colour(0xff0a0a0a));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);

    g.drawText("MIX", 100, 30, 100, 20, juce::Justification::centred);
    g.drawText("DECAY", 350, 30, 100, 20, juce::Justification::centred);
    g.drawText("CRUNCH", 600, 30, 100, 20, juce::Justification::centred);

    // Draw Preset Matrix Boundary
    g.setColour(juce::Colour(0xff1a1a1a));
    g.fillRoundedRectangle(40, 220, 720, 200, 10.0f);
    g.setColour(juce::Colour(0xff32cd32));
    g.drawRoundedRectangle(40, 220, 720, 200, 10.0f, 1.0f);
    
    clickZones.clear();

    int startX = 60;
    int startY = 240;
    int colWidth = 130;
    int rowHeight = 40;

    for (size_t col = 0; col < presetMatrix.size(); ++col)
    {
        g.setColour(juce::Colour(0xff32cd32));
        g.setFont(juce::Font(14.0f, juce::Font::bold));
        g.drawText(presetMatrix[col].title, startX + col * colWidth, startY, colWidth - 10, 20, juce::Justification::centredTop);

        g.setFont(13.0f);
        for (size_t row = 0; row < presetMatrix[col].presets.size(); ++row)
        {
            auto rect = juce::Rectangle<int>(startX + col * colWidth, startY + 30 + row * rowHeight, colWidth - 10, 30);
            
            g.setColour(juce::Colour(0xff1f1f1f));
            g.fillRect(rect);
            g.setColour(juce::Colours::white);
            g.drawText(presetMatrix[col].presets[row].name, rect, juce::Justification::centred);

            // Register click zone
            clickZones.push_back({rect, [this, col, row](const juce::MouseEvent& e) {
                if (e.mods.isRightButtonDown() && col == 4) // User col
                {
                    saveUserPreset((int)row);
                }
                else if (e.mods.isLeftButtonDown())
                {
                    loadPreset(presetMatrix[col].presets[row]);
                }
            }});
        }
    }
}

void RISeedIndustrialEditor::resized()
{
    mixSlider.setBounds (100, 60, 100, 120);
    decaySlider.setBounds (350, 60, 100, 120);
    crunchSlider.setBounds (600, 60, 100, 120);
}

void RISeedIndustrialEditor::mouseDown(const juce::MouseEvent& e)
{
    for (auto& zone : clickZones)
    {
        if (zone.first.contains(e.getPosition()))
        {
            zone.second(e);
            return;
        }
    }
}

void RISeedIndustrialEditor::loadPreset(const PresetInfo& info)
{
    if (auto* p = audioProcessor.apvts.getParameter("Mix")) p->setValueNotifyingHost(p->convertTo0to1(info.mix));
    if (auto* p = audioProcessor.apvts.getParameter("Decay")) p->setValueNotifyingHost(p->convertTo0to1(info.decay));
    if (auto* p = audioProcessor.apvts.getParameter("Crunch")) p->setValueNotifyingHost(p->convertTo0to1(info.crunch));
}

void RISeedIndustrialEditor::saveUserPreset(int slotIdx)
{
    float m = audioProcessor.apvts.getRawParameterValue("Mix")->load();
    float d = audioProcessor.apvts.getRawParameterValue("Decay")->load();
    float c = audioProcessor.apvts.getRawParameterValue("Crunch")->load();

    presetMatrix[4].presets[slotIdx].mix = m;
    presetMatrix[4].presets[slotIdx].decay = d;
    presetMatrix[4].presets[slotIdx].crunch = c;

    juce::Array<juce::var> arr;
    for (int i = 0; i < 3; ++i)
    {
        juce::DynamicObject::Ptr obj = new juce::DynamicObject();
        obj->setProperty("Mix", presetMatrix[4].presets[i].mix);
        obj->setProperty("Decay", presetMatrix[4].presets[i].decay);
        obj->setProperty("Crunch", presetMatrix[4].presets[i].crunch);
        arr.add(juce::var(obj.get()));
    }

    juce::File userDir = juce::File::getSpecialLocation(juce::File::userHomeDirectory).getChildFile("RISeed");
    userDir.createDirectory();
    
    juce::File userFile = userDir.getChildFile("user_presets.json");
    userFile.replaceWithText(juce::JSON::toString(arr));
}
