/*
  ==============================================================================

    DistortionClasses.h
    Created: 9 Nov 2021 11:36:19pm
    Author:  Max

  ==============================================================================
*/

#pragma once

#include <algorithm>
#include <cmath>

#include "TanhDistortion.h"
#include "SagEnvelope.h"
#include "LevelMeter.h"

class HypeClipper
{
public:

    //==========================================================================
    
    void prepare(
        double sampleRate,
        int numChannels)
    {
        tanh.prepare(sampleRate);
        sag.prepare(sampleRate);

        inputMeter.prepare(
            sampleRate,
            numChannels,
            50.0f);

        outputMeter.prepare(
            sampleRate,
            numChannels,
            50.0f);

        // Experimental HypeClipper sag settings.
        sag.setAttackMs(20.0f);
        sag.setReleaseMs(80.0f);
        sag.setDepth(0.50f);
    }

    //==========================================================================
    // Tanh parameters

    void setPreGain(float gain)
    {
        tanh.setPreGain(gain);
    }

    void setPostGain(float gain)
    {
        tanh.setPostGain(gain);
    }

    void setBias(float newBias)
    {
        tanh.setBias(newBias);
    }

    //==========================================================================
    // Sag parameters

    void setSagAttackMs(float milliseconds)
    {
        sag.setAttackMs(milliseconds);
    }

    void setSagReleaseMs(float milliseconds)
    {
        sag.setReleaseMs(milliseconds);
    }

    void setSagDepth(float newDepth)
    {
        sag.setDepth(newDepth);
    }

    //==========================================================================
    
    float processSample(float input)
    {
        //==============================================================
        // SAG EXPERIMENT
        //
        // Detector:
        //     input
        //
        // Application:
        //     before the tanh
        //
        // This is deliberately hard-coded for experimentation.

        const float sagEnvelope =
            sag.processSample(input);

        const float saggedInput =
            input / sagEnvelope;
        
        return sagEnvelope * tanh.processSample(saggedInput);
//        return tanh.processSample(input);
    }
    
    //==========================================================================
    // Process a block while measuring its input and output peaks.
    // The returned values are the current, smoothed meter values
    // in linear amplitude.

//    void processWithLevels(
//        float* buffer,
//        int numSamples)
//    {
//        float blockInputPeak = 0.0f;
//        float blockOutputPeak = 0.0f;
//
//        for (int i = 0; i < numSamples; ++i)
//        {
//            const float input = buffer[i];
//
//            blockInputPeak =
//                std::max(blockInputPeak,
//                         std::abs(input));
//
//            buffer[i] = processSample(input);
//
//            blockOutputPeak =
//                std::max(blockOutputPeak,
//                         std::abs(buffer[i]));
//        }
//
//        inputPeak = blockInputPeak;
//        outputPeak = blockOutputPeak;
//    }
    
    void processWithLevels(
        float** buffers,
        int numChannels,
        int numSamples)
    {
        for (int sample = 0;
             sample < numSamples;
             ++sample)
        {
            for (int channel = 0;
                 channel < numChannels;
                 ++channel)
            {
                const float input =
                    buffers[channel][sample];

                inputMeter.processSample(
                    input,
                    channel);

                const float output =
                    processSample(input);

                buffers[channel][sample] = output;

                outputMeter.processSample(
                    output,
                    channel);
            }

            inputMeter.processFrame();
            outputMeter.processFrame();
        }
    }
    
//    void updateLevelMeters(float& inPeakPlaceholder, float& outPeakPlaceholder, int blockSize){
//        inPeakPlaceholder = inputMeter.process(inputPeak, blockSize);
//        outPeakPlaceholder = outputMeter.process(outputPeak, blockSize);
//    }

    //==========================================================================
    
    void process(float* buffer, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
            buffer[i] = processSample(buffer[i]);
    }
    
    //==========================================================================
    float getInputLevelDb() const
    {
        return inputMeter.getLevelDb();
    }

    float getOutputLevelDb() const
    {
        return outputMeter.getLevelDb();
    }

private:

    TanhDistortion tanh;
    SagEnvelope sag;
    
    LevelMeter inputMeter;
    LevelMeter outputMeter;
    
    float inputPeak = 0.;
    float outputPeak = 0.;
};
