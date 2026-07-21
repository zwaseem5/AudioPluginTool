cmake_minimum_required(VERSION 3.22)

project(AudioPluginTool VERSION 0.1.0)

enable_testing()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

juce_add_plugin(AudioPluginTool
    COMPANY_NAME "Waseem"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "Audio Plugin Tool needs microphone access to process live audio input."
    PLUGIN_MANUFACTURER_CODE Wasm
    PLUGIN_CODE Apt1
    FORMATS VST3 Standalone
    PRODUCT_NAME "Audio Plugin Tool")

target_sources(AudioPluginTool
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/DSP/Tremolo.h
        Source/DSP/Clipper.h
        Source/DSP/Vocoder.h
        Source/DSP/PitchTracker.h
        Source/GUI/PluginLookAndFeel.h
        Source/GUI/PluginLookAndFeel.cpp)

target_compile_definitions(AudioPluginTool
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(AudioPluginTool
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Headless command-line tool that renders test signals through the plugin's
# AudioProcessor and writes WAV files for the Python test suite to analyse.
# Links against the AudioPluginTool shared-code target so it reuses the
# already-compiled PluginProcessor/DSP code instead of recompiling it.
add_executable(RenderHarness Source/RenderHarness/Main.cpp)
target_link_libraries(RenderHarness
    PRIVATE
        AudioPluginTool
        juce::juce_core
        juce::juce_audio_basics
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_dsp)

# C++ unit/integration tests (JUCE's UnitTest framework). These check
# structural correctness -- parameter IDs/ranges, DSP math in isolation,
# edge cases like empty buffers or sample-rate changes -- which complement
# the Python suite's acoustic/signal-level tests.
add_executable(UnitTests
    Source/Tests/Main.cpp
    Source/Tests/ParameterLayoutTests.cpp
    Source/Tests/DSPChainTests.cpp
    Source/Tests/TremoloTests.cpp
    Source/Tests/ClipperTests.cpp)
target_link_libraries(UnitTests
    PRIVATE
        AudioPluginTool
        juce::juce_core
        juce::juce_audio_basics
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_dsp)

add_test(NAME UnitTests COMMAND UnitTests)
