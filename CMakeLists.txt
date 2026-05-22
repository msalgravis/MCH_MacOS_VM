cmake_minimum_required(VERSION 3.22)

project(BinauralSpeakerRoom VERSION 0.1.0 LANGUAGES C CXX)

enable_testing()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(APPLE)
    set(BSR_MACOS_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum supported macOS version")
    set(BSR_MACOS_ARCHITECTURES "arm64;x86_64" CACHE STRING "Architectures for macOS builds")
    set(BSR_PLUGIN_FORMATS VST3 AU)

    set(CMAKE_OSX_DEPLOYMENT_TARGET "${BSR_MACOS_DEPLOYMENT_TARGET}" CACHE STRING "Minimum supported macOS version" FORCE)
    set(CMAKE_OSX_ARCHITECTURES "${BSR_MACOS_ARCHITECTURES}" CACHE STRING "Architectures for macOS builds" FORCE)
else()
    set(BSR_PLUGIN_FORMATS VST3)
endif()

include(FetchContent)

FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.2
)

FetchContent_MakeAvailable(juce)

juce_add_plugin(BinauralSpeakerRoom
    COMPANY_NAME "Sonarworks ltd"
    BUNDLE_ID com.sonarworks.vmpromch
    PLUGIN_MANUFACTURER_CODE BSRm
    PLUGIN_CODE VMCH
    FORMATS ${BSR_PLUGIN_FORMATS}
    PRODUCT_NAME "Sonarworks VMPRO MCH"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(BinauralSpeakerRoom
    PRIVATE
        Source/IRLoader.h
        Source/IRLoader.cpp
        Source/BinauralConvolver.h
        Source/BinauralConvolver.cpp
        Source/Parameters.h
        Source/Parameters.cpp
        Source/PluginProcessor.h
        Source/PluginProcessor.cpp
        Source/PluginEditor.h
        Source/PluginEditor.cpp
        Source/MchSlotIrManager.h
        Source/MchSlotIrManager.cpp
        Source/SpeakerPairProcessor.h
        Source/SpeakerPairProcessor.cpp
        Source/MchBinauralRenderer.h
        Source/MchBinauralRenderer.cpp
)

target_compile_definitions(BinauralSpeakerRoom
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

target_link_libraries(BinauralSpeakerRoom
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

juce_generate_juce_header(BinauralSpeakerRoom)

juce_add_console_app(BinauralSpeakerRoomTests
    PRODUCT_NAME "BinauralSpeakerRoomTests"
)

target_sources(BinauralSpeakerRoomTests
    PRIVATE
        Tests/TestMain.cpp
        Source/IRLoader.cpp
        Source/BinauralConvolver.cpp
        Source/Parameters.cpp
        Source/MchSlotIrManager.cpp
        Source/SpeakerPairProcessor.cpp
        Source/MchBinauralRenderer.cpp
)

target_include_directories(BinauralSpeakerRoomTests
    PRIVATE
        Source
        Tests
    ${CMAKE_CURRENT_BINARY_DIR}/BinauralSpeakerRoomTests_artefacts/JuceLibraryCode
)

target_link_libraries(BinauralSpeakerRoomTests
    PRIVATE
        juce::juce_audio_utils
    juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_definitions(BinauralSpeakerRoomTests
    PRIVATE
        BSR_PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}"
)

juce_generate_juce_header(BinauralSpeakerRoomTests)

add_test(NAME BinauralSpeakerRoomTests COMMAND BinauralSpeakerRoomTests)
