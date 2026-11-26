#include "PluginProcessor.h"
#include "PluginEditor.h"

CustomFaderLookAndFeel::CustomFaderLookAndFeel()
{
}

void CustomFaderLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                               float sliderPos, float, float,
                                               const juce::Slider::SliderStyle, juce::Slider& slider)
{
    const float capW = 44.0f;
    const float capH = 24.0f;
    const float capX = (float)x + ((float)width - capW) * 0.5f;
    const float capY = sliderPos - capH * 0.5f;

    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (capX + 1.0f, capY + 2.0f, capW, capH, 3.0f);

    grad (juce::Colour::fromRGB (85, 90, 98), capX, capY,
                               juce::Colour::fromRGB (25, 27, 30), capX, capY + capH, false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (capX, capY, capW, capH, 3.0f);

    g.setColour (juce::Colour::fromRGB (110, 115, 125));
    g.drawRoundedRectangle (capX, capY, capW, capH, 3.0f, 1.0f);

    g.setColour (juce::(240, 242, 245));(juce::Font::getDefaultMonospacedFontName(), 11.0f, juce::Font::bold));
    g.drawText (juce::String (slider.getValue(), 1), (int)capX, (int)capY, (int)capW, (int)capH, juce::Justification::centred, false);
}

UpwardCompressorAudioProcessorEditor::UpwardCompressorAudioProcessorEditor (UpwardCompressorAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (780, 580);

    auto setup = [this](jus) {        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
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
    // Fondo general
    g.fillAll (juce::Colour::fromRGB (16, 17, 20));

    // Panel Izquierdo
    juce::Rectangle<float> leftPanel (20.0f, 20.0f, 470.0f, 435.0f);
    g.setColour (juce::Colour::fromRGB (24, 26, 30));
    g.fillRoundedRectangle (leftPanel, 8.0f);
    g.setColour (juce::Colour::fromRGB (45, 48, 55));
    g.drawRoundedRectangle (leftPanel, 8.0f, 1.0f);

    // Panel Derecho
    juce::Rectangle<float> rightPanel (510.0f, 20.0f, 250.0f, 435.0f);
    g.setColour (juce::Colour::fromRGB (24, 26, 30));
    g.fillRoundedRectangle (rightPanel, 8.0f);
    g.setColour (juce::Colour::fromRGB (45, 48, 55));
    g.drawRoundedRectangle (rightPanel, 8.0f, 1.0f);

    // Titulos
    g.setColour (juce::(220, 225, 235));(juce::Font::getDefaultSansSerifFontName(), 13.0f,g.drawText ("LOW LEVEL",  20,  35, 235, 20, juce::Justification::centred, false);
    g.drawText ("HIGH LEVEL", 255, 35, 235, 20, juce::Justification::centred, false);
    g.drawText ("OUTPUT",     510, 35, 250, 20, juce::Justification::centred, false);

    const float trackY = 60.0f;
    const float trackH = 300.0f;

    // Pistas y Vumetros
    // Low Boost (Verde)
    juce::Rectangle<float> lowTrack (130.0f, trackY, 18.0f, trackH);
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (lowTrack, 2.0f);
    float lowFill = std::min (1.0f, std::max (0.0f, currentLowMeter / 48.0f)) * trackH;
    if (lowFill > 1.0f)
    {
        g.setColour (juce::Colour::fromRGB (52, 211, 153));
        g.fillRoundedRectangle (131.0f, trackY + trackH - lowFill, 16.0f, lowFill, 2.0f);
    }

    // High Reduccion (Rojo)
    juce::Rectangle<float> highTrack (350.0f, trackY, 18.0f, trackH);
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (highTrack, 2.0f);
    float highFill = std::min (1.0f,(0.0f, std::abs (currentHighMeter) / 48.0f)) * trackH;
    if (highFill > 1.0f)
    {
        g.setColour (juce::Colour::fromRGB (244, 63, 94));
        g.fillRoundedRectangle (351.0f, trackY, 16.0f, highFill, 2.0f);
    }

    // Output L y R (Ambar)
    auto drawOutBar = [&](float bx, float peak) {
        g.setColour (juce::Colours::black);
        g.fillRoundedRectangle (bx, trackY, 12.0f, trackH, 2.0f);
        float fill = std::min (1.0f, std::max (0.0f, peak)) * trackH;
        if (fill > 1.0f)
        {
            g.setColour (juce::Colour::fromRGB (245, 158, 11));
            g.fillRoundedRectangle (bx + 1.0f, trackY + trackH - fill, 10.0f, fill, 1.0f);
        }
    };
    drawOutBar (610.0f, currentPeakL);
    drawOutBar (638.0f, currentPeakR);

    // Cajas de lectura numerica
    auto drawBox = [&](int bx, int by, int bw, int bh, const jutext, juce::Colour col) {g.fillRoundedRectangle ((float)bx, (float)by, (float)bw, (float)bh, 3.0f);
        g.setColour (juce::Colour::fromRGB (50, 53, 60));
        g.drawRoundedRectangle ((float)bx, (float)by, (float)bw, (float)bh, 3.0f, 1.0f);
        g.setColour (col);
        g.setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(), 11.5f, juce::Font::bold));
        g.drawText (text, bx, by, bw, bh, juce::Justification::centred, false);
    };

    drawBox (108, 415, 62, 24, juce::String (lowSlider.getValue(), 1), juce::Colour::fromRGB (240, 245, 250));
    juhRed = (currentHighMeter <= -0.05f) ? juce::String (currentHighMeter, 1) : "0.0";
    drawBox (328, 415, 62, 24, hRed, juce::Colour::fromRGB (251, 113, 133));

    juce::String dL = (currentPeakL > 1e-4f) ? juce::String (20.0f * std::log10 (currentPeakL), 1) : "-inf";
    juce::String dR = (currentPeakR > 1e-4f) ? juce::String (20.0f * std::log10 (currentPeakR), 1) : "-inf";
    drawBox (590, 415, 48, 24, dL, juce::Colour::fromRGB (252, 211, 77));
    drawBox (642, 415, 48, 24, dR, juce::Colour::fromRGB (252, 211, 77));

    // Serigrafia
    g.setColour (juce::Colour::fromRGB (225, 228, 235));
    g.setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(), 14.0f, juce::Font::bold));
    g.drawText ("A  |  R", 0, 485, getWidth(), 20, juce::Justification::centred, false);

    g.setColour (juce::Colour::fromRGB (150, 155, 165));
    g.setFont (juce::Font (juce::Font::getDefaultSansSerifFontName(), 11.0f, juce::Font::bold));
    g.drawText ("UPWARD COMPRESSOR", 0, 515, getWidth(), 18, juce::Justification::centred, false);

    g.setColour (juce::Colour::fromRGB (100, 105, 115));
    g.setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(), 9.5f, juce::Font::plain));
  g.drawText ("Sonart.db", 0, 542, getWidth(), 16, juce::Justification::centred);
}

void UpwardCompressor{
    lowSlider.setBounds (115, 60, 48, 300);
    highSlider.setBounds (335, 60, 48, 300);
    outputSlider.setBounds (610, 60, 48, 300);
}
