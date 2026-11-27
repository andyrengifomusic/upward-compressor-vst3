#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

CustomFaderLookAndFeel::CustomFaderLookAndFeel() {}

void CustomFaderLookAndFeel::drawLinearSlider (jug, int x, int y, int width, int height,, float minSliderPos, float maxSliderPos,
                                               const juce::Slider::SliderStyle style,slider)Pos, style);

    const float capW = 54.0f;
    const float capH = 26.0f;
    const float capX = (float)x + ((float)width - capW) * 0.5f;
    const float capY = sliderPos - capH * 0.5f;

    // Sombra del fader cap
    g.setColour (juce::Colour::fromRGBA (0, 0, 0, 180));
    g.fillRoundedRectangle (capX + 1.0f, capY + 3.0f, capW, capH, 4.0f);

    // Bisel exterior metálico 3D
    juce::ColourGradient capGrad (juce::Colour::fromRGB (75, 80, 90), capX, capY,
                                 juce::Colour::fromRGB (28, 30, 35), capX, capY + capH, false);
    g.setGradientFill (capGrad);
    g.fillRoundedRectangle (capX, capY, capW, capH, 3.0f);

    // Borde brillante superior
    g.setColour (juce::Colour::fromRGBA (255, 255, 255, 50));
    g.drawRoundedRectangle (capX, capY, capW, capH, 3.0f, 1.0f);

    // Pantalla digital empotrada dentro del fader
    const float screenW = 42.0f;
    const float screenH = 18.0f;
    const float screenX = capX + (capW - screenW) * 0.5f;
    const float screenY = capY + (capH - screenH) * 0.5f;

    g.setColour (juce::Colour::fromRGB (10, 11, 13));
    g.fillRoundedRectangle (screenX, screenY, screenW, screenH, 2.0f);
    g.setColour (juce::Colour::fromRGB (50, 54, 62));
    g.drawRoundedRectangle (screenX, screenY, screenW, screenH, 2.0f, 1.0f);

    // Valor numérico iluminado
    g.setColour (juce::Colour::fromRGB (245, 248, 255));
    g.setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(), 11.0f, juce::Font::bold));
    g.drawText (juce::String (slider.getValue(), 1), (int)screenX, (int)screenY, (int)screenW, (int)screenH, juce::Justification::centred, false);
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
    currentPeakR     =(audioProcessor.meterPeakR.load(),     currentPeakR     * 0.88f);
    repaint();
}

void UpwardCompressorAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Chasis principal de aluminio oscuro
    juce::ColourGradient chassisGrad (juce::Colour::fromRGB (28, 30, 34), 0, 0,
                                      juce::Colour::fromRGB (14, 15, 17), 0, 580, false);
    g.setGradientFill (chassisGrad);
    g.fillAll();

    // Borde exterior del chasis
    g.setColour (juce::Colour::fromRGBA (255, 255, 255, 20));
    g.drawRect (0, 0, getWidth(), getHeight(), 1);

    // MÓDULO IZQUIERDO: LOW LEVEL + HIGH LEVEL
leftPanelGrad (juce::Colour::fromRGB (31, 33, 38), 24, 20,
                                        juce::Colour::fromRGB (18, 19, 22), 24, 460, false);
    g.setGradientFill (leftPanelGrad);
    g.fillRoundedRectangle (24.0f, 20.0f, 460.0f, 440.0f, 10.0f);
    g.setColour (juce::Colour::fromRGBA (255, 255, 255, 18));
    g.drawRoundedRectangle (24.0f, 20.0f, 460.0f, 440.0f, 10.0f, 1.0f);

    // Titulos superiores de secciones
    g.setColour (juce::Colour::fromRGB (220, 225, 235));
    g.setFont (juce::Font (juce::Font::getDefaultSansSerifFontName(), 13.0f, juce::Font::bold));
    g.drawText ("LOW LEVEL",  24,  32, 230, 20, juce::Justification::centred, false);
    g.drawText ("HIGH LEVEL", 254, 32, 230, 20, juce::Justification::centred, false);

    const float tY = 65.0f, tH = 300.0f;

    // Ranura de track para Low Level (con medidor de boost en verde-amarillo)
    g.setColour (juce::Colour::fromRGB (8, 9, 10));
    g.fillRoundedRectangle (130.0f, tY, 20.0f, tH, 3.0f);
    g.setColour (juce::Colour::fromRGB (45, 48, 55));
    g.drawRoundedRectangle (130.0f, tY, 20.0f, tH, 3.0f, 1.0f);

    float lH = std::min (1.0f, std::max (0.0f, currentLowMeter / 48.0f)) * tH;
    if (lH > 1.0f)
    {
        juce::ColourGradient lowMeterGrad (juce::Colour::fromRGB (253, 224, 71), 131, tY + tH - lH,
                                           juce::Colour::fromRGB (16, 185, 129), 131, tY + tH, false);
        g.setGradientFill (lowMeterGrad);
        g.fillRoundedRectangle (131.0f, tY + tH - lH, 18.0f, lH, 2.0f);
    }

    // Ranura de track para High Level (con medidor de atenuacion roja hacia abajo)
    g.setColour (juce::Colour::fromRGB (8, 9, 10));
    g.fillRoundedRectangle (350.0f, tY, 20.0f, tH, 3.0f);
    g.setColour (juce::Colour::fromRGB (45, 48, 55));
    g.drawRoundedRectangle (350.0f, tY, 20.0f, tH, 3.0f, 1.0f);

    float hH = std::min (1.0f, std::max (0.0f, std::abs (currentHighMeter) / 48.0f)) * tH;
    if (hH > 1.0f)
    {
        juce::ColourGradient highMeterGrad (juce::Colour::fromRGB (244, 63, 94), 351, tY,
                                            juce::Colour::fromRGB (185, 28, 28), 351, tY + hH, false);
        g.setGradientFill (highMeterGrad);
        g.fillRoundedRectangle (351.0f, tY, 18.0f, hH, 2.0f);
    }

    // Escalas de dB serigrafiadas (Low Level)
    const float lowTicks[] = { 48, 36, 24, 12, 6, 3, 0 };
    g.setColour (juce::Colour::fromRGB (160, 165, 175));
    g.setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(), 11.0f, juce::Font::plain));
    for (int i = 0; i < 7; ++i)
    {
        float norm = (48.0f - lowTicks[i]) / 48.0f;
        float yPos = tY + norm * tH;
        g.fillRect (155.0f, yPos - 1.0f, 8.0f, 2.0f);
        g.drawText (juce::String ((int)lowTicks[i]), 168, (int)yPos - 8, 30, 16, juce::Justification::left, false);
    }

    // Escalas de dB serigrafiadas (High Level)
    const float highTicks[] = { 0, -3, -6, -12, -24, -36, -48 };
    for (int i = 0; i < 7; ++i)
    {
        float norm = (0.0f - highTicks[i]) / 48.0f;
        float yPos = tY + norm * tH;
        g.fillRect (337.0f, yPos - 1.0f, 8.0f, 2.0f);
        g.drawText (juce::String ((int)highTicks[i]), 300, (int)yPos - 8, 32, 16, juce::Justification::right, false);
    }

    // Displays numéricos inferiores de Low y High
    g.setColour (juce::Colour::fromRGB (10, 11, 14));
    g.fillRoundedRectangle (105.0f, 385.0f, 70.0f, 28.0f, 4.0f);
    g.fillRoundedRectangle (325.0f, 385.0f, 70.0f, 28.0f, 4.0f);
    g.setColour (juce::Colour::fromRGB (50, 54, 62));
    g.drawRoundedRectangle (105.0f, 385.0f, 70.0f, 28.0f, 4.0f, 1.0f);
    g.drawRoundedRectangle (325.0f, 385.0f, 70.0f, 28.0f, 4.0f, 1.0f);

    float pLowVal = audioProcessor.apvts.getRawParameterValue ("lowLevel")->load();
    float pHighVal = audioProcessor.apvts.getRawParameterValue ("highLevel")->load();

    g.setColour (juce::Colour::fromRGB (240, 245, 255));
    g.setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::bold));
    g.drawText (juce::String (pLowVal, 1), 105, 385, 70, 28, juce::Justification::centred, false);
    g.drawText (juce::String (pHighVal, 1), 325, 385, 70, 28, juce::Justification::centred, false);

    // MÓDULO DERECHO: OUTPUT
    juce::ColourGradient rightPanelGrad (juce::Colour::fromRGB (28, 30, 35), 504, 20,
                                         juce::Colour::fromRGB (16, 17, 20), 504, 460, false);
    g.setGradientFill (rightPanelGrad);
    g.fillRoundedRectangle (504.0f, 20.0f, 252.0f, 440.0f, 10.0f);
    g.setColour (juce::Colour::fromRGBA (255, 255, 255, 18));
    g.drawRoundedRectangle (504.0f, 20.0f, 252.0f, 440.0f, 10.0f, 1.0f);

    // Tornillos anodizados decorativos en Output
    g.setColour (juce::Colour::fromRGB (120, 20, 30));
    g.fillEllipse (520.0f, 38.0f, 8.0f, 8.0f);
    g.fillEllipse (728.0f, 38.0f, 8.0f, 8.0f);

    g.setColour (juce::Colour::fromRGB (220, 225, 235));
    g.setFont (juce::Font (juce::Font::getDefaultSansSerifFontName(), 13.0f, juce::Font::bold));
    g.drawText ("OUTPUT", 504, 32, 252, 20, juce::Justification::centred, false);

    // Ranura doble de Output
    g.setColour (juce::Colour::fromRGB (8, 9, 10));
    g.fillRoundedRectangle (610.0f, tY, 14.0f, tH, 2.0f);
    g.fillRoundedRectangle (636.0f, tY, 14.0f, tH, 2.0f);
    g.setColour (juce::Colour::fromRGB (45, 48, 55));
    g.drawRoundedRectangle (610.0f, tY, 14.0f, tH, 2.0f, 1.0f);
    g.drawRoundedRectangle (636.0f, tY, 14.0f, tH, 2.0f, 1.0f);

    // Vúmetros estéreo de Output (Ámbar cálido)
    float pL =(1.0f, std::max (0.0f, currentPeakL)) * tH;
    float pR = std::min (1.0f, std::max (0.0f, currentPeakR)) * tH;

    if (pL > 1.0f)
    {
        juce::ColourGradient outMeterGrad (juce::Colour::fromRGB (254, 240, 138), 611, tY + tH - pL,
                                           juce::Colour::fromRGB (217, 119, 6), 611, tY + tH, false);
        g.setGradientFill (outMeterGrad);
        g.fillRoundedRectangle (611.0f, tY + tH - pL, 12.0f, pL, 1.0f);
    }

    if (pR > 1.0f)
    {
        juce::ColourGradient outMeterGrad (juce::Colour::fromRGB (254, 240, 138), 637, tY + tH - pR,
                                           juce::Colour::fromRGB (217, 119, 6), 637, tY + tH, false);
        g.setGradientFill (outMeterGrad);
        g.fillRoundedRectangle (637.0f, tY + tH - pR, 12.0f, pR, 1.0f);
    }

    // Escalas de Output (0, -3, -6, -12, -24, -36, -48, -60)
    const float outTicks[] = { 0, -3, -6, -12, -24, -36, -48, -60 };
    g.setColour (juce::Colour::fromRGB (160, 165, 175));
    g.setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(), 11.0f, juce::Font::plain));
    for (int i = 0; i < 8; ++i)
    {
        float norm = (0.0f - outTicks[i]) / 60.0f;
        float yPos = tY + norm * tH;
        g.fillRect (596.0f, yPos - 1.0f, 8.0f, 2.0f);
        g.drawText (juce::String ((int)outTicks[i]), 560, (int)yPos - 8, 32, 16, juce::Justification::right, false);
    }

    // Display digital inferior de Output
    g.setColour (juce::Colour::fromRGB (10, 11, 14));
    g.fillRoundedRectangle (595.0f, 385.0f, 70.0f, 28.0f, 4.0f);
    g.setColour (juce::Colour::fromRGB (50, 54, 62));
    g.drawRoundedRectangle (595.0f, 385.0f, 70.0f, 28.0f, 4.0f, 1.0f);

    float pOutVal = audioProcessor.apvts.getRawParameterValue ("outputGain")->load();
    g.setColour (juce::(245, 158, 11));ce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::bold));
    g.drawText (juce::String (pOutVal, 1), 595, 385, 70, 28, juce::Justification::centred, false);

    // PIE DE PÁGINA / FABRICANTE & BRANDING OFICIAL
    g.setColour (juce::Colour::fromRGB (230, 235, 245));
    g.setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(), 14.0f, juce::Font::bold));
    g.drawText ("A  |  R", 0, 480, getWidth(), 18, juce::Justification::centred, false);

    // Líneas separadoras elegantes
    juce::ColourGradient divGrad1 (juce::Colour::fromRGBA (255, 255, 255, 0), getWidth() * 0.5f - 120, 502,
                                  juce::Colour::fromRGBA (255, 255, 255, 50), getWidth() * 0.5f, 502, true);
    g.setGradientFill (divGrad1);
    g.fillRect ((int)(getWidth() * 0.5f - 120), 502, 240, 1);

    g.setColour (juce::Colour::fromRGB (160, 165, 175));
    g.setFont (juce::Font (juce::Font::getDefaultSansSerifFontName(), 11.0f, juce::Font::bold));
    g.drawText ("UPWARD COMPRESSOR", 0, 508, getWidth(), 16, juce::Justification::centred, false);

    // Firma oficial del Fabricante solicitada
    g.setColour (juce::Colour::fromRGB (190, 195, 205));
    g.setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(), 11.0f, juce::Font::plain));
    g.drawText ("Andy Rengifo | Sonart.dB", 0, 532, getWidth(), 18, juce::Justification::centred, false);
}

void UpwardCompressorAudioProcessorEditor::resized()
{
    // Posicionamiento preciso de los faders interactivos
    lowSlider.setBounds (100, 55, 80, 320);
    highSlider.setBounds (320, 55, 80, 320);
    outputSlider.setBounds (590, 55, 80, 320);
}
