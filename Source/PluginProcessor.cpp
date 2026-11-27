#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

UpwardCompressorAudioProcessor::UpwardCompressorAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::true)),())
{
}

UpwardCompressorAudioProcessor::~UpwardCompressorAudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout UpwardCompressorAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        ju{ "lowLevel", 1 },isableRange<float> (0.0f, 48.0f, 0.1f),
        6.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "highLevel", 1 },
        "High Level",
        juce::NormalisableRange<float> (-48.0f, 0.0f, 0.1f),
        -6.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "outputGain", 1 },
        "Output Gain",
        juce::NormalisableRange<float> (-60.0f, 0.0f, 0.1f),
        -0.5f));

    return { params.begin(), params.end() };
}

const juce::String UpwardCompressorAudioProcessor::getName() const
{
    return "Upward Compressor";
}

bool UpwardCompressorAudioProcessor::acceptsMidi() const
{
    return false;
}

bool UpwardCompressorAudioProcessor::producesMidi() const
{
    return false;
}

bool UpwardCompressorAudioProcessor::isMidiEffect() const
{
    return false;
}

double UpwardCompressorAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int UpwardCompressorAudioProcessor::getNumPrograms()
{
    return 1;
}

int UpwardCompressorAudioProcessor::getCurrentProgram()
{
    return 0;
}

void UpwardCompressorAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String UpwardCompressorAudioProcessor::getProgramName (int index)
{
    ju(index);ProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

void UpwardCompressorAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);

    const double sr = sampleRate > 0.0 ? sampleRate : 44100.0;
    lowAttCoeff  = static_cast<float> (std::exp (-1.0 / (20.0 * 0.001 * sr)));
    lowRelCoeff  = static_cast<float> (std::exp (-1.0 / (150.0 * 0.001 * sr)));
    highAttCoeff = static_cast<float> (std::exp (-1.0 / (5.0  * 0.001 * sr)));
    highRelCoeff = static_cast<float> (std::exp (-1.0 / (80.0 * 0.001 * sr)));

    lowEnv = 0.0f;
    highEnv = 0.0f;
    lowGainSmooth = 1.0f;
    highGainSmooth = 1.0f;
}

void UpwardCompressorAudioProcessor::releaseResources()
{
}

bool UpwardCompressorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void UpwardCompressorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;

    const auto totalNumInputChannels  = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    const int numSamples = buffer.getNumSamples();
    if (numSamples == 0 || totalNumInputChannels == 0)
        return;

    const float pLow  = apvts.getRawParameterValue ("lowLevel")->load();
    const float pHigh = apvts.getRawParameterValue ("highLevel")->load();
    const float pOut  = apvts.getRawParameterValue ("outputGain")->load();

    const float outGainLin = juce::Decibels::decibelsToGain (pOut);
    const float downwardRatio = 3.0f;
    const float downwardSlope = 1.0f - (1.0f / downwardRatio);

    auto* channelDataL = buffer.getWritePointer (0);
    auto* channelDataR = totalNumInputChannels > 1 ? buffer.getWritePointer (1) : channelDataL;

    float blockMaxBoost = 0.0f;
    float blockMaxAtten = 0.0f;
    float blockPeakL = 0.0f;
    float blockPeakR = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        const float dryL = channelDataL[i];
        const float dryR = channelDataR[i];
        const float detIn = std::max ((dryL), std::abs (dryR)); = lowAttCoeff * lowEnv + (1.0f - lowAttCoeff) * detIn;
        else
            lowEnv = lowRelCoeff * lowEnv + (1.0f - lowRelCoeff) * detIn;

        const float inDb = juce::Decibels::gainToDecibels (std::max (1e-6f, lowEnv));
        float targetLowBoostDb = 0.0f;

        if (pLow > 0.05f)
        {
            const float threshDb = -48.0f + pLow;
            if (inDb < threshDb)
            {
                const float depth = threshDb - inDb;
                const float boost = std::min (pLow, depth * 0.65f);
                const float peakSafety = (inDb >= -6.0f) ? 0.0f : std::max (0.0f, std::min (1.0f, (-6.0f - inDb) / 18.0f));
                targetLowBoostDb = boost * peakSafety;
            }
        }

        const float targetLowGainLin = juce::Decibels::decibelsToGain (targetLowBoostDb);
        if (targetLowGainLin > lowGainSmooth)
            lowGainSmooth = lowAttCoeff * lowGainSmooth + (1.0f - lowAttCoeff) * targetLowGainLin;
        else
            lowGainSmooth = lowRelCoeff * lowGainSmooth + (1.0f - lowRelCoeff) * targetLowGainLin;

        const float actualLowBoostDb = juce::Decibels::gainToDecibels (std::max (1e-6f, lowGainSmooth));
        blockMaxBoost = std::max (blockMaxBoost, actualLowBoostDb);

        if (detIn > highEnv)
            highEnv = highAttCoeff * highEnv + (1.0f - highAttCoeff) * detIn;
        else
            highEnv = highRelCoeff * highEnv + (1.0f - highRelCoeff) * detIn;

        const float highInDb = juce::Decibels::gainToDecibels (std::max (1e-6f, highEnv));
        float targetHighAttenDb = 0.0f;

        if (pHigh < -0.05f && highInDb > pHigh)
        {
            targetHighAttenDb = -downwardSlope * (highInDb - pHigh);
        }

        const float targetHighGainLin = juce::Decibels::decibelsToGain (targetHighAttenDb);
        if (targetHighGainLin < highGainSmooth)
            highGainSmooth = highAttCoeff * highGainSmooth + (1.0f - highAttCoeff) * targetHighGainLin;
        else
            highGainSmooth = highRelCoeff * highGainSmooth + (1.0f - highRelCoeff) * targetHighGainLin;

        const float actualHighAttenDb = juce::Decibels::gainToDecibels (std::max (1e-6f, highGainSmooth));
        blockMaxAtten = std::min (blockMaxAtten, actualHighAttenDb);

        const float totalGain = lowGainSmooth * highGainSmooth * outGainLin;
        const float outL = std::clamp (dryL * totalGain, -1.0f, 1.0f);
        const float outR = std::clamp (dryR * totalGain, -1.0f, 1.0f);

        channelDataL[i] = outL;
        channelDataR[i] = outR;

        blockPeakL = std::max (blockPeakL, std::abs (outL));
        blockPeakR = std::max (blockPeakR, std::abs (outR));
    }

    meterLowBoost.store (blockMaxBoost);
    meterHighAtten.store (blockMaxAtten);
    meterPeakL.store (blockPeakL);
    meterPeakR.store (blockPeakR);
}

bool UpwardCompressorAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* UpwardCompressorAudioProcessor::createEditor()
{
    return new UpwardCompressorAudioProcessorEditor (*this);
}

void UpwardCompressorAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void UpwardCompressorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new UpwardCompressorAudioProcessor();
}
