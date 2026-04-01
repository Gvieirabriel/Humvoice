cmake_minimum_required(VERSION 3.22)
project(Humvoice VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE path - adjust this to your JUCE installation
# You can also set JUCE_PATH environment variable or pass -DJUCE_PATH=/path/to/juce
if(DEFINED ENV{JUCE_PATH})
    set(JUCE_PATH $ENV{JUCE_PATH})
else()
    # Default path - change this to match your JUCE installation
    set(JUCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../juce-8.0.12-windows/JUCE")
endif()

# Check if JUCE path exists
if(NOT EXISTS "${JUCE_PATH}/CMakeLists.txt")
    message(FATAL_ERROR "JUCE not found at ${JUCE_PATH}. Please set JUCE_PATH environment variable or modify JUCE_PATH in CMakeLists.txt")
endif()

# Add JUCE as subdirectory
add_subdirectory("${JUCE_PATH}" JUCE)

# Create the VST3 plugin
juce_add_plugin(Humvoice
    PRODUCT_NAME "Humvoice"
    COMPANY_NAME "Humvoice"
    PLUGIN_MANUFACTURER_CODE Hmvc
    PLUGIN_CODE Hmv1
    FORMATS VST3
    VST3_CATEGORIES "Synth"
    DESCRIPTION "Minimal JUCE VST3 Plugin"
    NEEDS_MIDI_OUTPUT TRUE
)

# Disable VST2 to avoid parameter automation conflicts
target_compile_definitions(Humvoice
    PRIVATE
        JUCE_VST3_CAN_DO_SIDE_CHAIN=1
        JUCE_IGNORE_VST3_MISMATCHED_PARAMETER_ID_WARNING=1
)

# Add source files
target_sources(Humvoice
    PRIVATE
        src/AudioEngine.h
        src/AudioEngine.cpp
        src/PitchEngine.h
        src/PitchEngine.cpp
        src/MidiEngine.h
        src/MidiEngine.cpp
        src/DrumEngine.h
        src/DrumEngine.cpp
        src/PluginProcessor.h
        src/PluginProcessor.cpp
        src/PluginEditor.h
        src/PluginEditor.cpp
)

# Link JUCE
target_link_libraries(Humvoice
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
)

# Set include directories
target_include_directories(Humvoice
    PRIVATE
        src
        ${JUCE_MODULES_DIR}
)

# Enable IDE support
set_target_properties(Humvoice PROPERTIES 
    VS_GLOBAL_VcpkgEnabled false
)
