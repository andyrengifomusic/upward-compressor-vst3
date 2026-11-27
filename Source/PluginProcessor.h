#pragma once

#include <JuceHeader.h>

class UpwardCompressorAudioProcessor : public ju{
public:
    UpwardCompressorAudioProcessor();
    ~UpwardCompressorAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    std::atomic<float> meterLowBoost  { 0.0f };
    std::atomic<float> meterHighAtten { 0.0f };
    std::atomic<float> meterPeakL     { 0.0f };
    std::atomic<float> meterPeakR     { 0.0f };

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    float lowEnv = 0.0f;
    float highEnv = 0.0f;
    float lowGainSmooth = 1.0f;
    float highGainSmooth = 1.0f;

    float lowAttCoeff = 0.0f;
    float lowRelCoeff = 0.0f;
    float highAttCoeff = 0.0f;
    float highRelCoeff = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UpwardCompressorAudioProcessor)
};
