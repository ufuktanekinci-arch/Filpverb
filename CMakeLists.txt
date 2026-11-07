cmake_minimum_required(VERSION 3.22)
project(FlipVerb VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE wird beim ersten Konfigurieren automatisch von GitHub geladen.
# (Offline / eigener Ordner: -DFETCHCONTENT_SOURCE_DIR_JUCE=/pfad/zu/JUCE)
include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.15
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(FlipVerb
    COMPANY_NAME            "FlipVerb"
    PLUGIN_MANUFACTURER_CODE Flpv
    PLUGIN_CODE             Fvrb
    FORMATS                 VST3
    PRODUCT_NAME            "FlipVerb"
    IS_SYNTH                FALSE
    NEEDS_MIDI_INPUT        FALSE
    NEEDS_MIDI_OUTPUT       FALSE
    VST3_CATEGORIES         Fx Reverb
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(FlipVerb PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h)

target_include_directories(FlipVerb PRIVATE Source)

target_compile_definitions(FlipVerb PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_compile_options(FlipVerb PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

target_link_libraries(FlipVerb
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# ---------------------------------------------------------------------------
# Optionaler Offline-Test des Effekts (ohne DAW). Standardmaessig aus.
# ---------------------------------------------------------------------------
option(FLIPVERB_BUILD_TESTS "Offline DSP test bauen" OFF)
if(FLIPVERB_BUILD_TESTS)
    juce_add_console_app(FlipVerbTest PRODUCT_NAME "FlipVerbTest")
    target_sources(FlipVerbTest PRIVATE
        test/Test.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)
    target_include_directories(FlipVerbTest PRIVATE Source)
    target_compile_definitions(FlipVerbTest PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0)
    target_link_libraries(FlipVerbTest
        PRIVATE
            juce::juce_audio_utils
            juce::juce_dsp
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)
endif()
