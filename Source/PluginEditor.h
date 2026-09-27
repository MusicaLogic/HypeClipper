/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Style/VisualStyle.h"

//==============================================================================
class HypeClipperAudioProcessorEditor
    : public juce::AudioProcessorEditor,
      private juce::Timer
{
public:
    HypeClipperAudioProcessorEditor(
        HypeClipperAudioProcessor&);

    ~HypeClipperAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    //==============================================================================
    HypeClipperAudioProcessor& audioProcessor;

    //==========================================================================
    // Controls

    juce::Slider preSlider;
    juce::Slider biasSlider;
    juce::Slider postSlider;
    juce::Slider tubeSlider;

    juce::ToggleButton lockButton;
    
    // level meters
    juce::Slider inputMeter;
    juce::Slider outputMeter;

    //==========================================================================
    // Attachments

    std::unique_ptr<
        juce::AudioProcessorValueTreeState::SliderAttachment>
        preAttachment;

    std::unique_ptr<
        juce::AudioProcessorValueTreeState::SliderAttachment>
        biasAttachment;

    std::unique_ptr<
        juce::AudioProcessorValueTreeState::SliderAttachment>
        postAttachment;
    
    std::unique_ptr<
        juce::AudioProcessorValueTreeState::SliderAttachment>
        tubeAttachment;

    std::unique_ptr<
        juce::AudioProcessorValueTreeState::ButtonAttachment>
        lockAttachment;

    //==========================================================================
    // Waveform display

    class WaveformComponent
        : public juce::Component
    {
    public:
        explicit WaveformComponent(
            HypeClipperAudioProcessor& processor);

        void paint(juce::Graphics&) override;

        void setValues(
            float preDb,
            float bias,
            float postDB);

    private:
        HypeClipperAudioProcessor& processor;

        float preDb = 0.0f;
        float bias = 0.0f;
        float postDb = 0.0f;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
            WaveformComponent)
    };

    WaveformComponent waveform;

    //==========================================================================
    // Helpers

    void configureSlider(
        juce::Slider&,
        const juce::String& suffix);
    
    void configureMeter(
        juce::Slider& slider,
        const juce::String& suffix);

    void configureLockButton();

    void updateWaveform();

    void updatePostFromPre();

    float getLockedPost(float preDb) const;

    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        HypeClipperAudioProcessorEditor)
};
