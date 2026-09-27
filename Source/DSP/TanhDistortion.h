/*
  ==============================================================================

    DistortionClasses.h
    Created: 9 Nov 2021 11:36:19pm
    Author:  Max

  ==============================================================================
*/

#pragma once

#include <cmath>

class TanhDistortion
{
public:
    void prepare(double newSampleRate)
    {
        sampleRate = newSampleRate;
    }

    void setPreGain(float pre)
    {
        targetPreGain = pre;
    }

    void setPostGain(float post)
    {
        targetPostGain = post;
    }

    void setBias(float newBias)
    {
        targetBias = newBias;
    }

    float processSample(float input)
    {
        postGain += smoothingCoefficient * (targetPostGain - postGain);
        preGain += smoothingCoefficient * (targetPreGain - preGain);
        bias += 0.1*smoothingCoefficient * (targetBias - bias);
        return postGain
             * std::tanh(preGain * input + bias);
    }

    void process(float* buffer, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
            buffer[i] = processSample(buffer[i]);
    }

private:
    double sampleRate = 44100.0;

    float preGain  = 1.0f;
    float targetPreGain  = 1.0f;
    float postGain = 1.0f;
    float targetPostGain = 1.0f;
    float bias     = 0.0f;
    float targetBias = 0.0f;
    float smoothingCoefficient = 0.00005;
};
