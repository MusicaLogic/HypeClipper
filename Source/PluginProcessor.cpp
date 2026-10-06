/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
HypeClipperAudioProcessor::HypeClipperAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(
        BusesProperties()

       #if ! JucePlugin_IsMidiEffect
        #if ! JucePlugin_IsSynth
            .withInput(
                "Input",
                juce::AudioChannelSet::stereo(),
                true)
        #endif

            .withOutput(
                "Output",
                juce::AudioChannelSet::stereo(),
                true)
       #endif
    ),
    parameters(
        *this,
        nullptr,
        "PARAMETERS",
        createParameterLayout())
#endif
{
}

//==============================================================================
HypeClipperAudioProcessor::~HypeClipperAudioProcessor()
{
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
HypeClipperAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Pre gain
    params.push_back(
        std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "pre", 1 },
            "Pre",
            juce::NormalisableRange<float>(
                0.0f,
                48.0f,
                0.01f),
            0.0f));

    // Bias
    params.push_back(
        std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "bias", 1 },
            "Bias",
            juce::NormalisableRange<float>(
                0.0f,
                1.0f,
                0.001f),
            0.0f));
    
    // Tube / sag depth
    params.push_back(
        std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "tube", 1 },
            "Tube",
            juce::NormalisableRange<float>(
                0.0f,
                1.0f,
                0.001f),
            1.0f));

    // Post gain multiplier
    params.push_back(
        std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "post", 1 },
            "Post",
            juce::NormalisableRange<float>(
                -30.0f,
                0.0f,
                0.001f),
            0.0f));

    // UI state.
    //
    // The processor does not need to interpret this.
    // The editor uses it to determine whether Post is
    // controlled independently or through the Pre/Post
    // relationship.
    params.push_back(
        std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { "lock", 1 },
            "Lock",
            true));

    return {
        params.begin(),
        params.end()
    };
}

//==============================================================================
const juce::String HypeClipperAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool HypeClipperAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool HypeClipperAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool HypeClipperAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double HypeClipperAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

//==============================================================================
int HypeClipperAudioProcessor::getNumPrograms()
{
    return 1;
}

int HypeClipperAudioProcessor::getCurrentProgram()
{
    return 0;
}

void HypeClipperAudioProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String HypeClipperAudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void HypeClipperAudioProcessor::changeProgramName(
    int index,
    const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

//==============================================================================
void HypeClipperAudioProcessor::prepareToPlay(
    double sampleRate,
    int samplesPerBlock)
{
    juce::ignoreUnused(sampleRate, samplesPerBlock);
    
//    hypeClipper.prepare(sampleRate);
    hypeClipper.prepare(
            sampleRate,
            getTotalNumInputChannels());
    
    parameters.addParameterListener("pre", this);
    parameters.addParameterListener("bias", this);
    parameters.addParameterListener("post", this);
    parameters.addParameterListener("tube", this);
    
    parameters.addParameterListener("lock", this);
    
    hypeClipper.setPreGain(parameters.getRawParameterValue("pre")->load());
    hypeClipper.setPostGain(parameters.getRawParameterValue("post")->load());
    hypeClipper.setBias(parameters.getRawParameterValue("bias")->load());
    hypeClipper.setSagDepth(parameters.getRawParameterValue("tube")->load());
}

//==============================================================================
void HypeClipperAudioProcessor::releaseResources()
{
}

//==============================================================================
#ifndef JucePlugin_PreferredChannelConfigurations

bool HypeClipperAudioProcessor::isBusesLayoutSupported(
    const BusesLayout& layouts) const
{
   #if JucePlugin_IsMidiEffect

    juce::ignoreUnused(layouts);
    return true;

   #else

    if (layouts.getMainOutputChannelSet()
            != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet()
            != juce::AudioChannelSet::stereo())
    {
        return false;
    }

   #if ! JucePlugin_IsSynth

    if (layouts.getMainOutputChannelSet()
            != layouts.getMainInputChannelSet())
    {
        return false;
    }

   #endif

    return true;

   #endif
}

#endif

//==============================================================================
void HypeClipperAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);

    juce::ScopedNoDenormals noDenormals;
    
    float* channelPointers[2];

    for (int channel = 0;
         channel < buffer.getNumChannels();
         ++channel)
    {
        channelPointers[channel] =
            buffer.getWritePointer(channel);
    }

    hypeClipper.processWithLevels(
        channelPointers,
        buffer.getNumChannels(),
        buffer.getNumSamples());

    inputLevelDb.store(
        hypeClipper.getInputLevelDb());

    outputLevelDb.store(
        hypeClipper.getOutputLevelDb());
    
//    float inputPeak = 0.0f;
//    float outputPeak = 0.0f;
//    
//    for (int channel = 0;
//         channel < buffer.getNumChannels();
//         ++channel)
//    {
//        hypeClipper.processWithLevels(
//            buffer.getWritePointer(channel),
//            buffer.getNumSamples());
//    }
//    // update all 
//    hypeClipper.updateLevelMeters(inputPeak, outputPeak, buffer.getNumSamples());
//    
//    inputLevelDb.store(
//           juce::Decibels::gainToDecibels(inputPeak));
//    outputLevelDb.store(
//            juce::Decibels::gainToDecibels(outputPeak));

}

void HypeClipperAudioProcessor::parameterChanged(const juce::String& id, float newValue)
{
    if(id == "pre"){
        hypeClipper.setPreGain(
            juce::Decibels::decibelsToGain(newValue) );
    }else if(id == "bias"){
        hypeClipper.setBias(newValue);
    }else if(id == "post"){
        hypeClipper.setPostGain(
             juce::Decibels::decibelsToGain(newValue) );
    }else if (id == "tube")
    {
        // UI: 0...1
        // DSP sag depth: 0...0.5
        const float sagDepth = 0.99f * newValue;
        hypeClipper.setSagDepth(sagDepth);
    }
}

//==============================================================================
bool HypeClipperAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor*
HypeClipperAudioProcessor::createEditor()
{
    return new HypeClipperAudioProcessorEditor(*this);
}

//==============================================================================
void HypeClipperAudioProcessor::getStateInformation(
    juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();

    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void HypeClipperAudioProcessor::setStateInformation(
    const void* data,
    int sizeInBytes)
{
    auto xml = getXmlFromBinary(
        data,
        sizeInBytes);

    if (xml == nullptr)
        return;

    auto state =
        juce::ValueTree::fromXml(*xml);

    if (!state.isValid())
        return;

    parameters.replaceState(state);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HypeClipperAudioProcessor();
}
