#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class CustomFaderLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CustomFaderLookAndFeel();
    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle style, juce::Slider& slider) override;
};

class UpwardCompressorAudioProcessorEditor : public juce::AudioProcessorEditor,
                                            private juce::Timer
{
public:
    explicit UpwardCompressorAudioProcessorEditor (UpwardCompressorAudioProcessor&);
    ~UpwardCompressorAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    UpwardCompressorAudioProcessor& audioProcessor;
    CustomFaderLookAndFeel customLookAndFeel;

    juce::Slider lowSlider;
    juce::Slider highSlider;
    juce::Slider outputSlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lowAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> highAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputAttachment;

    float currentLowMeter  = 0.0f;
    float currentHighMeter = 0.0f;
    float currentPeakL     = 0.0f;
    float currentPeakR     = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UpwardCompressorAudioProcessorEditor)
};
