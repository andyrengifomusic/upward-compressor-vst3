#include "PluginProcessor.h"
#include "PluginEditor.h"

// ==========================================
// CUSTOM FADER LOOK AND FEEL
// ==========================================
CustomFaderLookAndFeel::CustomFaderLookAndFeel()
{
}

void CustomFaderLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                               float sliderPos, float /*minSliderPos*/, float /*maxSliderPos*/,
                                               const juce::Slider::SliderStyle /*style*/, juce::Slider& slider)
{
    // Solo dibujamos el cap (perilla deslizante) del fader, ya que la pista y vúmetros se renderizan en el fondo
    const float capWidth = 46.0f;
    const float capHeight = 26.0f;
    const float capX = (float)x + ((float)width - capWidth) * 0.5f;
    const float capY = sliderPos - capHeight * 0.5f;

    // Sombra del fader
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (capX + 1.0f, capY + 3.0f, capWidth, capHeight, 3.0f);

    // Cuerpo metálico del fader (gradiente de aluminio cepillado oscuro)
    capGrad (juce::Colour::fromRGB (90, 93, 100), capX, capY,
                                  juce::Colour::fromRGB (28, 30, 34), capX, capY + capHeight, false);
    g.setGradientFill (capGrad);
    g.fillRoundedRectangle (capX, capY, capWidth, capHeight, 3.0f);

    // Borde exterior del fader
    g.setColour (juce::Colour::fromRGB (115, 120, 130));
    g.drawRoundedRectangle (capX, capY, capWidth, capHeight, 3.0f, 1.0f);

    // Recuadro interior negro para lectura numérica
    const float displayW = 36.0f;
    const float displayH = 18.0f;
    const float displayX = capX + (capWidth - displayW) * 0.5f;
    const float displayY = capY + (capHeight - displayH) * 0.5f;

    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (displayX, displayY, displayW, displayH, 2.0f);
    g.setColour (juce::Colour::fromRGB (65, 70, 78));
    g.drawRoundedRectangle (displayX, displayY, displayW, displayH, 2.0f, 1.0f);

    // Valor numérico en blanco nítido
    g.setColour (juce::Colour::fromRGB (240, 242, 245));
    g.setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(), 11.5f, juce::Font::bold));
    juvalText = juce::String (slider.getValue(), 1);ittedText (valText, (int)displayX, (int)displayY, (int)displayW, (int)displayH, juce::Justification::centred, 1);
}

// ==========================================
// EDITOR COMPONENT
// ==========================================
UpwardCompressorAudioProcessorEditor::UpwardCompressorAudioProcessorEditor (UpwardCompressorAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (780, 580);

    // Configuración de los 3 Faders
    auto setupSlider = [this](juce::Slider& s) {
        s.setSliderStyle (juce::Slider::LinearVertical);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.setLookAndFeel (&customLookAndFeel);
        addAndMakeVisible (s);
    };

    setupSlider (lowSlider);
    setupSlider (highSlider);
    setupSlider (outputSlider);

    // Conexión con los parámetros del procesador
    lowAttachment    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "lowLevel",   lowSlider);
    highAttachment   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "highLevel",  highSlider);
    outputAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "outputGain", outputSlider);

    // Actualización de los vúmetros a 60 FPS
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
    // Obtener valores atómicos del audio thread
    float rawLowBoost  = audioProcessor.meterLowBoost.load();
    float rawHighAtten = audioProcessor.meterHighAtten.load();
    float rawPeakL     = audioProcessor.meterPeakL.load();
    float rawPeakR     = audioProcessor.meterPeakR.load();

    // Suavizado balístico para la pantalla
    currentLowMeter  = std::max (rawLowBoost, currentLowMeter * 0.92f);
    currentHighMeter = std::min (rawHighAtten, currentHighMeter * 0.90f);
    currentPeakL     = std::max (rawPeakL, currentPeakL * 0.88f);
    currentPeakR     = std::max (rawPeakR, currentPeakR * 0.88f);

    repaint();
}

void UpwardCompressorAudioProcessorEditor::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    // 1. Chasis exterior oscuro con textura y gradiente
    juce::ColourGradient bgGrad (juce::Colour::fromRGB (28, 30, 34), 0, 0,
                                 juce::Colour::fromRGB (14, 15, 17), 0, (float)getHeight(), false);
    g.setGradientFill (bgGrad);
    g.fillRect (bounds);

    g.setColour (juce::Colour::fromRGB (45, 48, 55));
    g.drawRect (bounds, 1.5f);

    // 2. Módulo Izquierdo (LOW LEVEL & HIGH LEVEL)
    juce::Rectangle<float> leftPanel (20.0f, 20.0f, 470.0f, 435.0f);
    juce::ColourGradient leftGrad (juce::Colour::fromRGB (31, 33, 38), leftPanel.getX(), leftPanel.getY(),
                                   juce::Colour::fromRGB (18, 19, 22), leftPanel.getX(), leftPanel.getBottom(), false);
    g.setGradientFill (leftGrad);
    g.fillRoundedRectangle (leftPanel, 10.0f);
    g.setColour (juce::Colour::fromRGB (45, 48, 55));
    g.drawRoundedRectangle (leftPanel, 10.0f, 1.0f);

    // Títulos de los Faders Izquierdos
    g.setColour (juce::Colour::fromRGB (225, 228, 235));
    g.setFont (juce::Font (juce::Font::getDefaultSansSerifFontName(), 13.0f, juce::Font::bold));
    g.drawFittedText ("LOW LEVEL",  (int)leftPanel.getX(), (int)leftPanel.getY() + 15, 235, 20, juce::Justification::centred, 1);
    g.drawFittedText ("HIGH LEVEL", (int)leftPanel.getX() + 235, (int)leftPanel.getY() + 15, 235, 20, juce::1);::Rectangle<float> rightPanel (510.0f, 20.0f, 250.0f, 435.0f);
    juce::ColourGradient rightGrad (juce::Colour::fromRGB (28, 30, 35), rightPanel.getX(), rightPanel.getY(),
                                    juce::Colour::fromRGB (17, 18, 21), rightPanel.getX(), rightPanel.getBottom(), false);
    g.setGradientFill (rightGrad);
    g.fillRoundedRectangle (rightPanel, 10.0f);
    g.setColour (juce::Colour::fromRGB (45, 48, 55));
    g.drawRoundedRectangle (rightPanel, 10.0f, 1.0f);

    // Luces indicadoras rojas y título OUTPUT
    g.setColour (juce::Colour::fromRGB (120, 20, 20));
    g.fillRoundedRectangle (rightPanel.getX() + 25.0f, rightPanel.getY() + 20.0f, 14.0f, 8.0f, 4.0f);
    g.fillRoundedRectangle (rightPanel.getRight() - 39.0f, rightPanel.getY() + 20.0f, 14.0f, 8.0f, 4.0f);

    g.setColour (juce::Colour::fromRGB (225, 228, 235));
    g.setFont (juce::Font (juce::Font::getDefaultSansSerifFontName(), 13.0f, juce::Font::bold));
    g.drawFittedText ("OUTPUT", (int)rightPanel.getX(), (int)rightPanel.getY() + 15, (int)rightPanel.getWidth(), 20, juce::Justification::centred, 1);

    // ==========================================
    // RENDERIZADO DE PISTAS Y VÚMETROS
    // ==========================================
    const float trackY = 55.0f;
    const float trackH = 300.0f;

    // --- PISTA 1: LOW LEVEL (Boost meter de abajo hacia arriba) ---
    const float lowTrackX = 130.0f;
    const float trackW = 18.0f;
    juce::Rectangle<float> lowTrackRect (lowTrackX, trackY, trackW, trackH);

    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (lowTrackRect, 2.0f);
    g.setColour (juce::Colour::fromRGB (50, 53, 60));
    g.drawRoundedRectangle (lowTrackRect, 2.0f, 1.0f);

    // Medidor Verde/Lima (Boost)
    float lowMeterNorm = std::min (1.0f, std::max (0.0f, currentLowMeter / 48.0f));
    float lowMeterFillH = lowMeterNorm * trackH;
    if (lowMeterFillH > 1.0f)
    {
        juce::Rectangle<float> lowMeterRect (lowTrackX + 1.0f, trackY + trackH - lowMeterFillH, trackW - 2.0f, lowMeterFillH);
        juce::ColourGradient lowMeterGrad (juce::Colour::fromRGB (16, 185, 129), lowMeterRect.getX(), trackY + trackH,
                                           juce::Colour::fromRGB (250, 204, 21), lowMeterRect.getX(), lowMeterRect.getY(), false);
        g.setGradientFill (lowMeterGrad);
        g.fillRoundedRectangle (lowMeterRect, 2.0f);
    }

    // --- PISTA 2: HIGH LEVEL (Gain Reduction meter de arriba hacia abajo) ---
    const float highTrackX = 350.0f;
    juce::Rectangle<float> highTrackRect (highTrackX, trackY, trackW, trackH);

    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (highTrackRect, 2.0f);
    g.setColour (juce::Colour::fromRGB (50, 53, 60));
    g.drawRoundedRectangle (highTrackRect, 2.0f, 1.0f);

    // Medidor Rojo (Reducción)
    float highMeterNorm = std::min (1.0f, std::max (0.0f, std::abs (currentHighMeter) / 48.0f));
    float highMeterFillH = highMeterNorm * trackH;
    if (highMeterFillH > 1.0f)
    {
        juce::Rectangle<float> highMeterRect (highTrackX + 1.0f, trackY, trackW - 2.0f, highMeterFillH);
        juce::ColourGradient highMeterGrad (juce::Colour::fromRGB (244, 63, 94), highMeterRect.getX(), trackY,
                                            juce::Colour::fromRGB (185, 28, 28), highMeterRect.getX(), trackY + highMeterFillH, false);
        g.setGradientFill (highMeterGrad);
        g.fillRoundedRectangle (highMeterRect, 2.0f);
    }

    // --- PISTA 3: OUTPUT (Doble medidor ámbar L/R) ---
    const float outMeterW = 12.0f;
    const float outTrackL_X = 610.0f;
    const float outTrackR_X = 638.0f;

    auto drawOutputMeter = [&](float meterX, float peakVal) {
        juce::Rectangle<float> mRect (meterX, trackY, outMeterW, trackH);
        g.setColour (g.fillRoundedRectangle (mRect, 2.0f);
        g.setColour (juce::Colour::fromRGB (50, 53, 60));
        g.drawRoundedRectangle (mRect, 2.0f, 1.0f);

        float norm = std::min (1.0f, std::max (0.0f, peakVal));
        float fillH = norm * trackH;
        if (fillH > 1.0f)
        {
            juce::Rectangle<float> fillRect (meterX + 1.0f, trackY + trackH - fillH, outMeterW - 2.0f, fillH);
outGrad (juce::Colour::fromRGB (217, 119, 6), fillRect.getX(), trackY + trackH,
                                          juce::Colour::fromRGB (253, 230, 138), fillRect.getX(), fillRect.getY(), false);
            g.setGradientFill (outGrad);
            g.fillRoundedRectangle (fillRect, 1.0f);
        }
    };

    drawOutputMeter (outTrackL_X, currentPeakL);
    drawOutputMeter (outTrackR_X, currentPeakR);

    // ==========================================
    // ESCALAS GRADUADAS EN DECIBELIOS (dB)
    // ==========================================
    g.setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(), 11.0f, juce::Font::plain));
    g.setColour (juce::Colour::fromRGB (170, 175, 185));

    // Low Level scale (48, 36, 24, 12, 6, 3, 0)
    const std::vector<std::pair<float, juce::String>> lowScale = {
        { 0.0f, "48" }, { 0.25f, "36" }, { 0.50f, "24" }, { 0.75f, "12" }, { 0.875f, "6" }, { 1.0f, "0" }
    };
    for (const auto& tick : lowScale)
    {
        float yPos = trackY + tick.first * trackH;
        g.fillRect (lowTrackX + trackW + 6.0f, yPos - 0.5f, 6.0f, 1.0f);
        g.drawSingleLineText (tick.second, (int)(lowTrackX + trackW + 16.0f), (int)yPos + 4);
    }

    // High Level scale (0, -3, -6, -12, -24, -36, -48)
    const std::vector<std::pair<float, juce::String>> highScale = {
        { 0.0f, "0" }, { 0.125f, "-6" }, { 0.25f, "-12" }, { 0.50f, "-24" }, { 0.75f, "-36" }, { 1.0f, "-48" }
    };
    for (const auto& tick : highScale)
    {
        float yPos = trackY + tick.first * trackH;
        g.fillRect (highTrackX - 12.0f, yPos - 0.5f, 6.0f, 1.0f);
        g.drawSingleLineText (tick.second, (int)(highTrackX - 38.0f), (int)yPos + 4);
    }

    // Output scale (0, -6, -12, -24, -36, -48, -60)
    const std::vector<std::pair<float, juce::String>> outScale = {
        { 0.0f, "0" }, { 0.1f, "-6" }, { 0.2f, "-12" }, { 0.4f, "-24" }, { 0.6f, "-36" }, { 0.8f, "-48" }, { 1.0f, "-60" }
    };
    for (const auto& tick : outScale)
    {
        float yPos = trackY + tick.first * trackH;
        g.fillRect (outTrackL_X - 10.0f, yPos - 0.5f, 6.0f, 1.0f);
        g.drawSingleLineText (tick.second, (int)(outTrackL_X - 36.0f), (int)yPos + 4);
    }

    // ==========================================
    // CAJAS DE LECTURA DIGITAL INFERIORES
    // ==========================================
    auto drawDigitalBox = [&](float boxX, float boxY, float boxW, float boxH, const juce::String& text, juce::Colour textCol) {
        g.setColour (g.fillRoundedRectangle (boxX, boxY, boxW, boxH, 3.0f);
        g.setColour (juce::Colour::fromRGB (55, 58, 65));
        g.drawRoundedRectangle (boxX, boxY, boxW, boxH, 3.0f, 1.0f);

        g.setColour (textCol);
        g.setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::bold));
        g.drawFittedText (text, (int)boxX, (int)boxY, (int)boxW, (int)boxH, juce::Justification::centred, 1);
    };

    const float readY = 410.0f;
    const float readW = 65.0f;
    const float readH = 26.0f;

    // Lectura Low Level
    drawDigitalBox (lowTrackX - 22.0f, readY, readW, readH,
(lowSlider.getValue(), 1), juce::Colour::fromRGB (240, 245, 250));

    // Lectura High Level Gain Reduction
    juce::String highRedText = (currentHighMeter <= -0.05f) ? juce::String (currentHighMeter, 1) : "0.0";
    drawDigitalBox (highTrackX - 22.0f, readY, readW, readH,
                    highRedText, juce::Colour::fromRGB (251, 113, 133));

    // Lecturas Output L y R
    juce::String dbL = (currentPeakL > 1e-4f) ? juce::String (20.0f * std::log10 (currentPeakL), 1) : "-inf";
dbR = (currentPeakR > 1e-4f) ? juce::String (20.0f * std::log10 (currentPeakR), 1) : "-inf";

    drawDigitalBox (outTrackL_X - 25.0f, readY, 52.0f, readH, dbL, juce::Colour::fromRGB (252, 211, 77));
    drawDigitalBox (outTrackR_X + 2.0f,  readY, 52.0f, readH, dbR, juce::Colour::fromRGB (252, 211, 77));

    // ==========================================
    // LOGOS Y SERIGRAFÍA DEL HARDWARE
    // ==========================================
    const float centerFooterX = (float)getWidth() * 0.5f;

    // Logo principal "A | R"
    g.setColour (juce::Colour::fromRGB (230, 233, 240));
    g.setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(), 14.0f, juce::Font::bold));
    g.drawFittedText ("A  |  R", 0, 485, getWidth(), 20, juce::Justification::centred, 1);

    // Línea separadora 1
    g.setColour (juce::(60, 64, 72)); - 100.0f, 510.0f, centerFooterX + 100.0f, 510.0f, 1.0f);

    // Texto "UPWARD COMPRESSOR"
    g.setColour (juce::Colour::fromRGB (160, 165, 175));
    g.setFont (juce::Font (juce::Font::getDefaultSansSerifFontName(), 11.5f, juce::Font::bold));
    g.drawFittedText ("UPWARD COMPRESSOR", 0, 516, getWidth(), 18, juce::Justification::centred, 1);

    // Línea separadora 2
    g.setColour (juce::Colour::fromRGB (50, 54, 60));
    g.drawLine (centerFooterX - 60.0f, 538.0f, centerFooterX + 60.0f, 538.0f, 1.0f);

    // Marca de agua "Sonart.db"
    g.setColour (juce::Colour::fromRGB (110, 115, 125));
    g.setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(), 10.0f, juce::Font::plain));
    g.drawFittedText ("Sonart.db", 0, 544, getWidth(), 16, juce::Justification::centred, 1);
}

void UpwardCompressorAudioProcessorEditor::resized()
{
    const int trackY = 55;
    const int trackH = 300;

    // Posicionamiento de los faders interactivos sobre sus pistas y vúmetros
    lowSlider.setBounds (130 - 15, trackY, 48, trackH);
    highSlider.setBounds (350 - 15, trackY, 48, trackH);
    outputSlider.setBounds (620 - 15, trackY, 50, trackH);
}
