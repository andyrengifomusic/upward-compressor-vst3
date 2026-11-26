#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>

class UpwardCompressorAudioProcessor : public juce::AudioProcessor
{
public:
    UpwardCompressorAudioProcessor();
    ~UpwardCompressorAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Upward Compressor"; }

    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    std::atomic<float> meterLowBoost { 0.0f };
    std::atomic<float> meterHighAtten { 0.0f };
    std::atomic<float> meterPeakL { 0.0f };
    std::atomic<float> meterPeakR { 0.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    float lowEnv = 0.0f;
    float highEnv = 0.0f;
    float lowGainSmooth = 1.0f;
    float highGainSmooth = 1.0f;

    float lowAttCoeff = 0.0f;
    float lowRelCoeff = 0.0f;
    float highAttCoeff = 0.0f;
    float highRelCoeff = 0.0f;

    static constexpr float downwardRatio = 3.0f;
    static constexpr float downwardSlope = 1.0f - (1.0f / downwardRatio);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UpwardCompressorAudioProcessor)
};
