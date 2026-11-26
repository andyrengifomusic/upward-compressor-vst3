#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <algorithm>

UpwardCompressorAudioProcessor::UpwardCompressorAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

UpwardCompressorAudioProcessor::~UpwardCompressorAudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout UpwardCompressorAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Low Level: 0.0 a 48.0 dB (Default: 6.0 dB)
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "lowLevel", 1 },
        "Low Level",
        juce::NormalisableRange<float> (0.0f, 48.0f, 0.1f),
        6.0f));

    // High Level: -48.0 a 0.0 dB (Default: -6.0 dB)
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "highLevel", 1 },
        "High Level",
        juce::NormalisableRange<float> (-48.0f, 0.0f, 0.1f),
        -6.0f));

    // Output Gain: -60.0 a 0.0 dB (Default: -0.5 dB)
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "outputGain", 1 },
        "Output Gain",
        juce::NormalisableRange<float> (-60.0f, 0.0f, 0.1f),
        -0.5f));

    return { params.begin(), params.end() };
}

void UpwardCompressorAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    const float sr = static_cast<float> (sampleRate > 0.0 ? sampleRate : 44100.0);

    // Coeficientes de envolvente (tiempos de ataque y relajación)
    lowAttCoeff  = std::exp (-1.0f / (0.020f * sr)); // 20 ms ataque upward
    lowRelCoeff  = std::exp (-1.0f / (0.150f * sr)); // 150 ms relajación upward
    highAttCoeff = std::exp (-1.0f / (0.005f * sr)); // 5 ms ataque downward
    highRelCoeff = std::exp (-1.0f / (0.080f * sr)); // 80 ms relajación downward

    lowEnv = 0.0f;
    highEnv = 0.0f;
    lowGainSmooth = 1.0f;
    highGainSmooth = 1.0f;

    meterLowBoost.store (0.0f);
    meterHighAtten.store (0.0f);
    meterPeakL.store (0.0f);
    meterPeakR.store (0.0f);
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

void UpwardCompressorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, numSamples);

    if (numSamples == 0 || totalNumInputChannels == 0)
        return;

    const float pLow  = apvts.getRawParameterValue ("lowLevel")->load();
    const float pHigh = apvts.getRawParameterValue ("highLevel")->load();
    const float pOut  = apvts.getRawParameterValue ("outputGain")->load();

    const float outGainLin = std::pow (10.0f, pOut / 20.0f);

    float* channelDataL = buffer.getWritePointer (0);
    float* channelDataR = (totalNumInputChannels > 1) ? buffer.getWritePointer (1) : nullptr;

    float maxBlockBoost = 0.0f;
    float maxBlockAtten = 0.0f;
    float peakOutL = 0.0f;
    float peakOutR = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        const float dryL = channelDataL[i];
        const float dryR = (channelDataR != nullptr) ? channelDataR[i] : dryL;
        const float detIn = std::max (std::abs (dryL), std::abs (dryR));

        // 1. DETECCIÓN Y COMPRESIÓN UPWARD (LOW LEVEL)
        if (detIn > lowEnv)
            lowEnv = lowAttCoeff * lowEnv + (1.0f - lowAttCoeff) * detIn;
        else
            lowEnv = lowRelCoeff * lowEnv + (1.0f - lowRelCoeff) * detIn;

        const float inDb = 20.0f * std::log10 (std::max (1e-6f, lowEnv));
        float targetLowBoostDb = 0.0f;

        if (pLow > 0.05f)
        {
            const float threshDb = -48.0f + pLow;
            if (inDb < threshDb)
            {
                const float depth = threshDb - inDb;
                const float boost = std::min (pLow, depth * 0.65f);
                // Protección para picos: no altera transientes por encima de -6 dB
                const float peakSafety = (inDb >= -6.0f) ? 0.0f : std::max (0.0f, std::min (1.0f, (-6.0f - inDb) / 18.0f));
                targetLowBoostDb = boost * peakSafety;
            }
        }

        const float targetLowGainLin = std::pow (10.0f, targetLowBoostDb / 20.0f);
        if (targetLowGainLin > lowGainSmooth)
            lowGainSmooth = lowAttCoeff * lowGainSmooth + (1.0f - lowAttCoeff) * targetLowGainLin;
        else
            lowGainSmooth = lowRelCoeff * lowGainSmooth + (1.0f - lowRelCoeff) * targetLowGainLin;

        const float actualLowBoostDb = 20.0f * std::log10 (std::max (1e-6f, lowGainSmooth));
        maxBlockBoost = std::max (maxBlockBoost, actualLowBoostDb);

        // 2. DETECCIÓN Y COMPRESIÓN DOWNWARD (HIGH LEVEL)
        if (detIn > highEnv)
            highEnv = highAttCoeff * highEnv + (1.0f - highAttCoeff) * detIn;
        else
            highEnv = highRelCoeff * highEnv + (1.0f - highRelCoeff) * detIn;

        const float highInDb = 20.0f * std::log10 (std::max (1e-6f, highEnv));
        float targetHighAttenDb = 0.0f;

        if (pHigh < -0.05f)
        {
            if (highInDb > pHigh)
                targetHighAttenDb = -downwardSlope * (highInDb - pHigh);
        }

        const float targetHighGainLin = std::pow (10.0f, targetHighAttenDb / 20.0f);
        if (targetHighGainLin < highGainSmooth)
            highGainSmooth = highAttCoeff * highGainSmooth + (1.0f - highAttCoeff) * targetHighGainLin;
        else
            highGainSmooth = highRelCoeff * highGainSmooth + (1.0f - highRelCoeff) * targetHighGainLin;

        const float actualHighAttenDb = 20.0f * std::log10 (std::max (1e-6f, highGainSmooth));
        maxBlockAtten = std::min (maxBlockAtten, actualHighAttenDb);

        // 3. COMBINACIÓN DINÁMICA Y SALIDA
        const float totalDynamics = lowGainSmooth * highGainSmooth;
        const float outLVal = std::max (-1.0f, std::min (1.0f, dryL * totalDynamics * outGainLin));
        const float outRVal = std::max (-1.0f,(1.0f, dryR * totalDynamics * outGainLin));[i] = outLVal;
        if (channelDataR != nullptr)
            channelDataR[i] = outRVal;

        peakOutL = std::max (peakOutL, std::abs (outLVal));
        peakOutR = std::max (peakOutR, std::abs (outRVal));
    }

    meterLowBoost.store (maxBlockBoost);
    meterHighAtten.store (maxBlockAtten);
    meterPeakL.store (peakOutL);
    meterPeakR.store (peakOutR);
}

bool UpwardCompressorAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* UpwardCompressorAudioProcessor::createEditor()
{
    return new UpwardCompressorAudioProcessorEditor (*this);
}

const juce::String UpwardCompressorAudioconstconst         { return false; }cesMidi() const        { return false; }
bool UpwardCompressorAudioProcessor::isMidiEffect() const        { return false; }
double UpwardCompressorAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int UpwardCompressorAudioProcessor::getNumPrograms()            { return 1; }
int UpwardCompressorAudioProcessor::getCurrentProgram()         { return 0; }
void UpwardCompressorAudioProcessor::setCurrentProgram (int)    {}
const juce::String UpwardCompressorAudio(int) { return {}; } (int, const juce::String&) {}

void UpwardCompressorAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void UpwardCompressorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new UpwardCompressorAudioProcessor();
}
