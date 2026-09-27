/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// WaveformComponent
//==============================================================================

HypeClipperAudioProcessorEditor::WaveformComponent::WaveformComponent(
    HypeClipperAudioProcessor& p)
    : processor(p)
{
    setOpaque(false);
}

//==============================================================================
void HypeClipperAudioProcessorEditor::WaveformComponent::setValues(
    float newPreDb,
    float newBias,
    float newPostDb)
{
    preDb = newPreDb;
    bias = newBias;
    postDb = newPostDb;

    repaint();
}

//==============================================================================
void HypeClipperAudioProcessorEditor::WaveformComponent::paint(
    juce::Graphics& g)
{
    auto bounds =
        getLocalBounds().toFloat().reduced(12.0f);

    // Panel
    g.setColour(VisualStyle::panelBackground);
    g.fillRoundedRectangle(
        bounds,
        VisualStyle::Geometry::componentCornerRadius);

    g.setColour(
        VisualStyle::Palette::red.dim);

    g.drawRoundedRectangle(
        bounds,
        VisualStyle::Geometry::componentCornerRadius,
        VisualStyle::Geometry::componentBorderThickness);

    //==========================================================================
    // Center line

    const float centreY = bounds.getCentreY();

    g.setColour(
        VisualStyle::Palette::red.dim.withAlpha(0.35f));

    g.drawHorizontalLine(
        juce::roundToInt(centreY),
        bounds.getX() + 10.0f,
        bounds.getRight() - 10.0f);

    //==========================================================================
    // Generate conceptual waveform
    //
    // x -> input sine
    // pre -> input drive
    // bias -> vertical offset inside tanh
    // post -> output scaling

    juce::Path waveformPath;

    constexpr int numPoints = 300;

    const float preGain =
        juce::Decibels::decibelsToGain(preDb);
    const float postGain =
        juce::Decibels::decibelsToGain(postDb);

    for (int i = 0; i < numPoints; ++i)
    {
        const float t =
            static_cast<float>(i)
            / static_cast<float>(numPoints - 1);

        const float x =
            bounds.getX() + t * bounds.getWidth();

        // 20 cycles.
        const float phase =
            (1.0f - t) * juce::MathConstants<float>::twoPi * 20.0f;

        const float input =
            std::sin(phase);
        
        const float transformed =
            postGain * std::tanh( t * preGain * input + bias );
        
//        DBG("phase: " << phase << " - input: " << input << " - transformed: " << transformed);

        // Display scaling.
        //
        // Keep the waveform inside the panel even
        // when Post becomes large.
        const float displayGain = 0.5f; // 0.32f;

        const float y =
            centreY
            - transformed
                * bounds.getHeight()
                * displayGain;

        if (i == 0)
            waveformPath.startNewSubPath(x, y);
        else
            waveformPath.lineTo(x, y);
    }

    //==========================================================================
    // Waveform

    g.setColour(
        VisualStyle::Palette::red.highlight);

    g.strokePath(
        waveformPath,
        juce::PathStrokeType(
            2.0f,
            juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded));

    //==========================================================================
    // Small center marker

    g.setColour(
        VisualStyle::Palette::red.highlight
            .withAlpha(0.5f));

    g.fillEllipse(
        bounds.getCentreX() - 2.0f,
        centreY - 2.0f,
        4.0f,
        4.0f);
}


//==============================================================================
// Editor
//==============================================================================

HypeClipperAudioProcessorEditor::
HypeClipperAudioProcessorEditor(
    HypeClipperAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      waveform(p)
{
    //==========================================================================
    // Sliders

    addAndMakeVisible(preSlider);
    configureSlider(preSlider, "dB");

    addAndMakeVisible(biasSlider);
    configureSlider(biasSlider, "bias");

    addAndMakeVisible(postSlider);
    configureSlider(postSlider, "dB");
    
    addAndMakeVisible(tubeSlider);
    configureSlider(tubeSlider, "tube");
    

    //==========================================================================
    // Lock

    addAndMakeVisible(lockButton);
    configureLockButton();
    
    //==========================================================================
    // Level meters
    addAndMakeVisible(inputMeter);
    configureMeter(inputMeter, "dB");

    addAndMakeVisible(outputMeter);
    configureMeter(outputMeter, "dB");

    //==========================================================================
    // Waveform

    addAndMakeVisible(waveform);

    //==========================================================================
    // Attachments

    preAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::SliderAttachment>(
                audioProcessor.parameters,
                "pre",
                preSlider);

    biasAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::SliderAttachment>(
                audioProcessor.parameters,
                "bias",
                biasSlider);

    postAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::SliderAttachment>(
                audioProcessor.parameters,
                "post",
                postSlider);
    
    tubeAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::SliderAttachment>(
                audioProcessor.parameters,
                "tube",
                tubeSlider);

    lockAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::ButtonAttachment>(
                audioProcessor.parameters,
                "lock",
                lockButton);

    //==========================================================================
    // Initial state

    updatePostFromPre();
    updateWaveform();

//    // A lightweight timer is sufficient for the visual relationship.
    startTimerHz(30);
    
    // instead of timer, use callbacks for slider updates
    preSlider.onValueChange = [this]
    {
        updatePostFromPre();
        updateWaveform();
    };
    
    biasSlider.onValueChange = [this]
    {
        updateWaveform();
    };

    postSlider.onValueChange = [this]
    {
        updateWaveform();
    };

    // Landscape.
    setSize(800, 400);
}

//==============================================================================
HypeClipperAudioProcessorEditor::
~HypeClipperAudioProcessorEditor()
{
    stopTimer();
}

//==============================================================================
void HypeClipperAudioProcessorEditor::configureSlider(
    juce::Slider& slider,
    const juce::String& suffix)
{
    slider.setSliderStyle(
        juce::Slider::RotaryVerticalDrag);

    slider.setTextBoxStyle(
        juce::Slider::TextBoxBelow,
        false,
        80,
        20);

    //==========================================================================
    // Red visual style

    slider.setColour(
        juce::Slider::textBoxTextColourId,
        VisualStyle::Palette::red.highlight);

    slider.setColour(
        juce::Slider::textBoxOutlineColourId,
        juce::Colours::transparentBlack);

    slider.setColour(
        juce::Slider::rotarySliderFillColourId,
        VisualStyle::Palette::red.highlight);

    slider.setColour(
        juce::Slider::rotarySliderOutlineColourId,
        VisualStyle::Palette::red.dim);

    slider.setColour(
        juce::Slider::thumbColourId,
        juce::Colours::transparentBlack);

    slider.setTextValueSuffix(
        suffix.isEmpty()
            ? juce::String()
            : " " + suffix);

    slider.setNumDecimalPlacesToDisplay(2);
}

//==============================================================================
void HypeClipperAudioProcessorEditor::configureMeter(
    juce::Slider& slider,
    const juce::String& suffix)
{
    slider.setSliderStyle(
        juce::Slider::LinearVertical);

    slider.setRange(-60.0, 6.0, 0.01);
    slider.setValue(-60.0);

    slider.setTextBoxStyle(
        juce::Slider::TextBoxBelow,
        true,       // read-only
        70,
        20);

    slider.setTextValueSuffix(
        " " + suffix);

    slider.setNumDecimalPlacesToDisplay(1);

    slider.setColour(
        juce::Slider::textBoxTextColourId,
        VisualStyle::Palette::red.dim);

    slider.setColour(
        juce::Slider::textBoxOutlineColourId,
        juce::Colours::transparentBlack);

    slider.setColour(
        juce::Slider::trackColourId,
        VisualStyle::Palette::red.disabled);

    slider.setColour(
        juce::Slider::backgroundColourId,
        VisualStyle::background);

    slider.setColour(
        juce::Slider::thumbColourId,
        VisualStyle::Palette::red.dim);

    // The meter is display-only.
    slider.setInterceptsMouseClicks(false, false);
}

//==============================================================================
void HypeClipperAudioProcessorEditor::configureLockButton()
{
    lockButton.setButtonText("LOCK");

    lockButton.setClickingTogglesState(true);

    // Completely transparent JUCE button appearance.
    // The editor paints the visual representation.
    lockButton.setColour(
        juce::ToggleButton::textColourId,
        juce::Colours::transparentBlack);

    lockButton.setColour(
        juce::ToggleButton::tickColourId,
        juce::Colours::transparentBlack);

    lockButton.setColour(
        juce::ToggleButton::tickDisabledColourId,
        juce::Colours::transparentBlack);

    lockButton.setColour(
        juce::ComboBox::backgroundColourId,
        juce::Colours::transparentBlack);

    lockButton.setColour(
        juce::ComboBox::outlineColourId,
        juce::Colours::transparentBlack);
    
    lockButton.onStateChange = [this] {
        updatePostFromPre();
    };
}

//==============================================================================
// Lock relationship
//==============================================================================

float HypeClipperAudioProcessorEditor::getLockedPost(
    float preDb) const
{
    return -16*(preDb/48);
//    return -0.6f*preDb;
}

//==============================================================================
void HypeClipperAudioProcessorEditor::updatePostFromPre()
{
    if (!lockButton.getToggleState())
    {
        postSlider.setEnabled(true);
        return;
    }

    postSlider.setEnabled(false);

    const float pre =
        static_cast<float>(preSlider.getValue());

    const float post =
        getLockedPost(pre);

    postSlider.setValue(
        post,
        juce::sendNotificationSync);
}

//==============================================================================
void HypeClipperAudioProcessorEditor::updateWaveform()
{
    waveform.setValues(
        static_cast<float>(preSlider.getValue()),
        static_cast<float>(biasSlider.getValue()),
        static_cast<float>(postSlider.getValue()));
}

//==============================================================================
void HypeClipperAudioProcessorEditor::timerCallback()
{
//    updatePostFromPre();
//    updateWaveform();
    
    inputMeter.setValue(
        audioProcessor.getInputLevelDb());

    outputMeter.setValue(
        audioProcessor.getOutputLevelDb());

    repaint();
}

//==============================================================================
// Painting
//==============================================================================

void HypeClipperAudioProcessorEditor::paint(
    juce::Graphics& g)
{
    g.fillAll(
        VisualStyle::background);

    auto bounds =
        getLocalBounds().reduced(2);

    // Main panel
    g.setColour(
        VisualStyle::panelBackground);

    g.fillRoundedRectangle(
        bounds.toFloat(),
        VisualStyle::Geometry::componentCornerRadius);

    g.setColour(
        VisualStyle::Palette::red.dim);

    g.drawRoundedRectangle(
        bounds.toFloat(),
        VisualStyle::Geometry::componentCornerRadius,
        VisualStyle::Geometry::componentBorderThickness);

    //==========================================================================
    // Title

    g.setColour(
        VisualStyle::Palette::red.highlight);

    g.setFont(
        VisualStyle::getDefaultFont(
            VisualStyle::FontSize::title));
    
    //==============================================================================
    // Pre <-> Post connection
    //
    // This is deliberately painted BEFORE the LOCK button.
    // The button therefore covers the middle of the line.

    const auto preBounds =
        preSlider.getBounds();

    const auto postBounds =
        postSlider.getBounds();

    const int lineY =
        lockButton.getBounds().getCentreY();

    const float preX =
        static_cast<float>(preBounds.getCentreX());

    const float postX =
        static_cast<float>(postBounds.getCentreX());

    const float stemHeight = 7.0f;

    const bool locked =
        lockButton.getToggleState();

    const auto lineColour =
        locked
            ? VisualStyle::Palette::red.highlight
            : VisualStyle::Palette::red.dim;

    const float lineThickness =
        locked ? 2.0f : 1.0f;

    g.setColour(lineColour);

    // Horizontal connection
    g.drawLine(
        preX,
        static_cast<float>(lineY),
        postX,
        static_cast<float>(lineY),
        lineThickness);

    //==============================================================================
    // Small vertical connection stems

    g.drawLine(
        preX,
        static_cast<float>(lineY) - stemHeight,
        preX,
        static_cast<float>(lineY) + stemHeight,
        lineThickness);

    g.drawLine(
        postX,
        static_cast<float>(lineY) - stemHeight,
        postX,
        static_cast<float>(lineY) + stemHeight,
        lineThickness);
    
    //==============================================================================
    // LOCK button
    //
    // The line is painted first, so the button visually sits on top of it.

    const auto lockBounds =
        lockButton.getBounds().toFloat();

//    const bool locked =
//        lockButton.getToggleState();

    const auto buttonBackground =
        locked
            ? VisualStyle::Palette::red.highlight
            : VisualStyle::panelBackground;

    const auto buttonText =
        locked
            ? VisualStyle::background
            : VisualStyle::Palette::red.highlight;

    const auto buttonBorder =
        locked
            ? VisualStyle::Palette::red.highlight
            : VisualStyle::Palette::red.dim;

    // Background
    g.setColour(buttonBackground);

    g.fillRoundedRectangle(
        lockBounds,
        VisualStyle::Geometry::buttonCornerRadius);

    // Border
    g.setColour(buttonBorder);

    g.drawRoundedRectangle(
        lockBounds,
        VisualStyle::Geometry::buttonCornerRadius,
        VisualStyle::Geometry::buttonBorderThickness);

    // Text
    g.setColour(buttonText);

    g.setFont(
        VisualStyle::getDefaultFont(
            VisualStyle::FontSize::normal));

    g.drawText(
        "LOCK",
        lockBounds.toNearestInt(),
        juce::Justification::centred);
}

//==============================================================================
void HypeClipperAudioProcessorEditor::resized()
{
//    auto area = getLocalBounds().reduced(24);
    auto area = getLocalBounds();

    //==========================================================================
    // Title area

//    const int titleHeight = 42;
//
//    auto titleArea =
//        area.removeFromTop(titleHeight);
//
//    juce::ignoreUnused(titleArea);

    //==========================================================================
    // Waveform

    const int waveformHeight = 155;

    auto waveformArea =
        area.removeFromTop(waveformHeight);

    waveform.setBounds(
        waveformArea.reduced(10, 4));

    //==========================================================================
    // Controls

    const int controlsTop = 12;

    area.removeFromTop(controlsTop);
    
    // Vertical division:
    // Level meters: 2+2 = 4
    // Pre/Post: 8+8 = 16
    // Bias/Tube: 4+4 = 8
    // Gaps: level-1-pre-2-bias-3-tube-4-post-5-level = 5
    // Total: 33
    
    const int columnGap = area.getWidth()/33.0f;
    
//    const int columnGap = 30;
//
//    const int columnWidth =
//        (area.getWidth() - 2 * columnGap) / 3;
    
    auto inLevelArea =
        area.removeFromLeft(3*columnGap);
    
//    area.removeFromLeft(columnGap);
    
    auto preArea =
        area.removeFromLeft(8*columnGap);

    area.removeFromLeft(columnGap);

    auto biasArea =
        area.removeFromLeft(4*columnGap);

    area.removeFromLeft(columnGap);
    
    auto tubeArea =
        area.removeFromLeft(4*columnGap);

    area.removeFromLeft(columnGap);

    auto postArea =
        area.removeFromLeft(8*columnGap);
    
//    area.removeFromLeft(columnGap);
    
    auto outLevelArea =
        area.removeFromLeft(3*columnGap);

    //==========================================================================
    // Knobs

    const int bigKnobHeight =
        juce::jmin(150, area.getHeight() - 35);
    
    const int smallKnobHeight =
        juce::jmin(130, area.getHeight() - 55);
    
    inputMeter.setBounds(
        inLevelArea.withHeight(bigKnobHeight));
    
    preSlider.setBounds(
        preArea.withHeight(bigKnobHeight));

    biasSlider.setBounds(
        biasArea.withHeight(smallKnobHeight));
    
    tubeSlider.setBounds(
        tubeArea.withHeight(smallKnobHeight));

    postSlider.setBounds(
        postArea.withHeight(bigKnobHeight));
    
    outputMeter.setBounds(
        outLevelArea.withHeight(bigKnobHeight));

    //==========================================================================
    // Lock button
    //
    // It deliberately sits below the knob row.

    const int buttonWidth = 96;
    const int buttonHeight = 30;

    const int lockX =
        getWidth() / 2 - buttonWidth / 2;

    const int lockY =
        getHeight() - 48;

    lockButton.setBounds(
        lockX,
        lockY,
        buttonWidth,
        buttonHeight);
}
