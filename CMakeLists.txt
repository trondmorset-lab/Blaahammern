cmake_minimum_required(VERSION 3.22)

project(T1revPRO VERSION 1.8.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Universal binary (Apple Silicon + Intel) og macOS 10.13+
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "" FORCE)
endif()

# JUCE: bruk lokal kopi hvis JUCE_DIR er satt, ellers lastes den ned automatisk
set(JUCE_DIR "" CACHE PATH "Sti til en lokal JUCE-mappe (valgfritt)")
if(JUCE_DIR)
    add_subdirectory(${JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 9.0.6
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()

set(PLUGIN_FORMATS VST3)
if(APPLE)
    list(APPEND PLUGIN_FORMATS AU)
endif()

# Kodene (Kfio/Fkl1) er de samme som i første versjon, så lagrede prosjekter
# finner pluginen igjen.
juce_add_plugin(T1revPRO
    COMPANY_NAME "Morset Labs"
    BUNDLE_ID "no.morsetlabs.t1revpro"
    PLUGIN_MANUFACTURER_CODE Kfio
    PLUGIN_CODE Fkl1
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "T1 revPRO"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/assets/t1revpro-logo-1024.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/assets/t1revpro-logo-256.png"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES Fx Reverb
    AU_MAIN_TYPE kAudioUnitType_Effect
    COPY_PLUGIN_AFTER_BUILD FALSE)

juce_add_binary_data(T1revPROAssets NAMESPACE LogoData SOURCES assets/t1revpro-logo-256.png)

set(T1_KILDER
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/RomSyntese.cpp
    Source/Analysator.cpp)

target_sources(T1revPRO PRIVATE ${T1_KILDER})
target_include_directories(T1revPRO PRIVATE Source)

target_compile_definitions(T1revPRO PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(T1revPRO
    PRIVATE
        T1revPROAssets
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Valgfrie tester (av som standard): cmake -DT1REVPRO_BUILD_TESTS=ON
# T1revPROTest kjører prosessoren uten DAW (decay, diffusion, EQ, RTA,
# tilstand) og tegner GUI-et til screenshot.png og screenshot-decay.png.
option(T1REVPRO_BUILD_TESTS "Bygg testprogrammet" OFF)
if(T1REVPRO_BUILD_TESTS)
    juce_add_console_app(T1revPROTest PRODUCT_NAME "T1revPROTest")
    target_sources(T1revPROTest PRIVATE Tests/T1revPROTest.cpp ${T1_KILDER})
    target_include_directories(T1revPROTest PRIVATE Source)
    target_compile_definitions(T1revPROTest PRIVATE
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_DISPLAY_SPLASH_SCREEN=0 T1REVPRO_TEST=1)
    target_link_libraries(T1revPROTest
        PRIVATE T1revPROAssets juce::juce_audio_utils juce::juce_dsp
        PUBLIC juce::juce_recommended_config_flags)
endif()
