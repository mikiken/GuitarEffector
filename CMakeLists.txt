cmake_minimum_required(VERSION 3.22)
project(GuitarEffector VERSION 1.0.0)

# Export compile_commands.json for C++ IDEs (VS Code, clangd, Antigravity IDE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(JUCE)

# Define JUCE application (standalone)
juce_add_gui_app(GuitarEffector
    PRODUCT_NAME "Guitar Effector"
    COMPANY_NAME "mikiken"
    BUNDLE_ID "com.mikiken.guitareffector"
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "Needs audio input for processing guitar signal."
)

juce_generate_juce_header(GuitarEffector)

target_sources(GuitarEffector PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp
    Source/MainComponent.h
    Source/EffectBlock/EffectBlock.h
    Source/EffectBlock/GainBlock.h
)

# Link JUCE libraries
target_link_libraries(GuitarEffector PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra

    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
