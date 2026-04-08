cmake_minimum_required(VERSION 3.20)
project(RISeed-Industrial VERSION 1.0.0)

add_subdirectory(JUCE)

juce_add_plugin(RISeed-Industrial
    COMPANY_NAME "Ghost Note"
    BUNDLE_ID "com.GhostNote.RISeedIndustrial"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE "Risi"
    PLUGIN_CODE "Rsdi"
    FORMATS VST3
    PRODUCT_NAME "RISeed-Industrial")

# Force output directories as requested for CI
set_target_properties(RISeed-Industrial PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/RISeed-Industrial_artefacts"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/RISeed-Industrial_artefacts"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/RISeed-Industrial_artefacts"
)

# Also apply this to make sure Mac/Win formats use the folder
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/RISeed-Industrial_artefacts")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/RISeed-Industrial_artefacts")

target_compile_features(RISeed-Industrial PUBLIC cxx_std_20)

target_include_directories(RISeed-Industrial PRIVATE 
    CloudSeedCore 
    CloudSeedCore/DSP 
    Source
)

target_sources(RISeed-Industrial PRIVATE
    CloudSeedCore/Parameters.cpp
    CloudSeedCore/DSP/Biquad.cpp
    CloudSeedCore/DSP/RandomBuffer.cpp

    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/AutomationWatcher.cpp
)

target_link_libraries(RISeed-Industrial PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_core
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_dsp
)

target_compile_definitions(RISeed-Industrial PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

juce_generate_juce_header(RISeed-Industrial)
