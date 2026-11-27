#include "PluginProcessor.h"
#include "PluginEditor.h"

CustomFaderLookAndFeel::CustomFaderLookAndFeel() {}

void CustomFaderLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                               float sliderPos, float minSliderPos, float maxSliderPos,
                                               const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    juce::ignoreUnused (minSliderPos, maxSliderPos, style);

    const float capW = 46.0f;
    const float capH = 24.0f;
    const float capX = (float)x + ((float)width - capW) * 0.5f;
    const float capY = sliderPos - capH * 0.5f;

    // Fondo del cap
    g.setColour (juce::Colour::fromRGB (28, 30, 34));
    g.fillRoundedRectangle (capX, capY, capW, capH, 3.0f);

    // Borde
    g.setColour (juce::Colour::fromRGB (110, 115, 125));
    g.drawRoundedRectangle (capX, capY, capW, capH, 3.0f, 1.0f);

    // Texto del valor
    g.setColour (juce::Colour::fromRGB (240, 242, 245));
    g.setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(), 11.0f, juce::Font::bold));
    g.drawText (juce::String (slider.getValue(), 1), (int)capX, (int)capY, (int)capW, (int)capH, juce::Justification::centred, false);
}

UpwardCompressorAudioProcessorEditor::UpwardCompressorAudioProcessorEditor (UpwardCompressorAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (780, 580);

    auto setup = [this](juce::Slider& s) {
        s.setSliderStyle (juce::Slider::LinearVertical);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.setLookAndFeel (&customLookAndFeel);
        addAndMakeVisible (s);
    };

    setup (lowSlider);
    setup (highSlider);
    setup (outputSlider);

    lowAttachment    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "lowLevel",   lowSlider);
    highAttachment   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "highLevel",  highSlider);
    outputAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "outputGain", outputSlider);

    startTimerHz (60);
}

UpwardCompressorAudioProcessorEditor::~UpwardCompressorAudioProcessorEditor()
{
    stopTimer();
    lowSlider.setLookAndFeel (nullptr);
    highSlider.setLookAndFeel (nullptr);
    outputSlider.setLookAndFeel (nullptr);
}

void UpwardCompressorAudioProcessorEditor::timerCallback()
{
    currentLowMeter  = std::max (audioProcessor.meterLowBoost.load(),  currentLowMeter  * 0.92f);
    currentHighMeter = std::min (audioProcessor.meterHighAtten.load(), currentHighMeter * 0.90f);
    currentPeakL     = std::max (audioProcessor.meterPeakL.load(),     currentPeakL     * 0.88f);
    currentPeakR     = std::max (audioProcessor.meterPeakR.load(),     currentPeakR     * 0.88f);
    repaint();
}

void UpwardCompressorAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour::fromRGB (16, 17, 20));

    // Paneles
    g.setColour (juce::Colour::fromRGB (24, 26, 30));
    g.fillRoundedRectangle (20.0f, 20.0f, 470.0f, 435.0f, 8.0f);
    g.fillRoundedRectangle (510.0f, 20.0f, 250.0f, 435.0f, 8.0f);

    // Titulos
    g.setColour (juce::Colour::fromRGB (220, 225, 235));
    g.setFont (juce::Font (juce::Font::getDefaultSansSerifFontName(), 13.0f, juce::Font::bold));
    g.drawText ("LOW LEVEL",  20,  35, 235, 20, juce::Justification::centred, false);
    g.drawText ("HIGH LEVEL", 255, 35, 235, 20, juce::Justification::centred, false);
    g.drawText ("OUTPUT",     510, 35, 250, 20, juce::Justification::centred, false);

    const float tY = 60.0f, tH = 300.0f;

    // Low meter (Verde)
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (130.0f, tY, 18.0f, tH, 2.0f);
    float lH = std::min (1.0f, std::max (0.0f, currentLowMeter / 48.0f)) * tH;
    g.setColour (juce::Colour::fromRGB (52, 211, 153));
    if (lH > 1.0f) 
        g.fillRoundedRectangle (131.0f, tY + tH - lH, 16.0f, lH, 2.0f);

    // High meter (Rojo)
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (350.0f, tY, 18.0f, tH, 2.0f);
    float hH = std::min (1.0f, std::max (0.0f, std::abs (currentHighMeter) / 48.0f)) * tH;
    g.setColour (juce::Colour::fromRGB (244, 63, 94));
    if (hH > 1.0f) 
        g.fillRoundedRectangle (351.0f, tY, 16.0f, hH, 2.0f);

    // Output meters (Ambar)
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (610.0f, tY, 12.0f, tH, 2.0f);
    g.fillRoundedRectangle (638.0f, tY, 12.0f, tH, 2.0f);
    g.setColour (juce::Colour::fromRGB (245, 158, 11));
    float pL = std::min (1.0f,(0.0f, currentPeakL)) * tH;
    float pR = std::min (1.0f, std::max (0.0f, currentPeakR)) * tH;
    if (pL > 1.0f) 
        g.fillRoundedRectangle (611.0f, tY + tH - pL, 10.0f, pL, 1.0f);
    if (pR > 1.0f) 
        g.fillRoundedRectangle (639.0f, tY + tH - pR, 10.0f, pR, 1.0f);

    // Logos
    g.setColour (juce::Colour::fromRGB (225, 228, 235));
    g.setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(), 14.0f, juce::Font::bold));
    g.drawText ("A  |  R", 0, 485, getWidth(), 20, juce::Justification::centred, false);
    g.drawText ("UPWARD COMPRESSOR", 0, 515, getWidth(), 18, juce::Justification::centred, false);
    g.drawText ("Sonart.db", 0, 542, getWidth(), 16, juce::Justification::centred, false);
}

void UpwardCompressorAudioProcessorEditor::resized()
{
    lowSlider.setBounds (115, 60, 48, 300);
    highSlider.setBounds (335, 60, 48, 300);
    outputSlider.setBounds (610, 60, 48, 300);
}
