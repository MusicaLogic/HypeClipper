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
#include <vector>

class SagEnvelope
{
public:

    //==========================================================================
    void prepare(double newSampleRate)
    {
        sampleRate = newSampleRate;

        updateRmsWindow();

        rmsHistory.assign(
            static_cast<size_t>(rmsWindowSamples),
            0.0f);

        rmsWritePosition = 0;
        rmsSumSquares = 0.0;

        envelope = 1.0f;

        updateCoefficients();
    }

    //==========================================================================
    // Sag attack/release are specified in milliseconds.

    void setAttackMs(float milliseconds)
    {
        attackMs = std::max(0.01f, milliseconds);
        updateCoefficients();
    }

    void setReleaseMs(float milliseconds)
    {
        releaseMs = std::max(0.01f, milliseconds);
        updateCoefficients();
    }

    //==========================================================================
    // RMS detector window.
    //
    // This determines over what period the signal's energy is estimated.
    //
    // Shorter:
    //     more responsive
    //
    // Longer:
    //     smoother / more power-like

    void setRmsWindowMs(float milliseconds)
    {
        rmsWindowMs = std::max(0.1f, milliseconds);
        updateRmsWindow();
    }

    //==========================================================================
    // 0.0 = no sag
    // 1.0 = maximum possible sag

    void setDepth(float newDepth)
    {
        depth = std::clamp(newDepth, 0.0f, 1.0f);
    }

    //==========================================================================
    // Process one sample.
    //
    // The input signal is first converted into an RMS-based estimate of
    // signal demand. The sag envelope then follows the corresponding target.

    float processSample(float input)
    {
        //==============================================================
        // RMS detector
        //
        // Use the square of the input because power / energy is related
        // to signal squared.

        const float squared =
            input * input;

        const float oldSquared =
            rmsHistory[
                static_cast<size_t>(rmsWritePosition)];

        rmsHistory[
            static_cast<size_t>(rmsWritePosition)] =
            squared;

        rmsSumSquares +=
            static_cast<double>(squared)
            - static_cast<double>(oldSquared);

        ++rmsWritePosition;

        if (rmsWritePosition >= rmsWindowSamples)
            rmsWritePosition = 0;

        const float rms =
            std::sqrt(
                static_cast<float>(
                    std::max(
                        0.0,
                        rmsSumSquares
                        / static_cast<double>(
                            rmsWindowSamples))));

        //==============================================================
        // Convert signal demand into sag target.
        //
        // rms = 0:
        //     target = 1
        //
        // rms = 1 and depth = 1:
        //     target = 0

        const float level =
            std::pow(std::clamp(10*rms, 0.0f, 0.8f), 5.0);

        const float target =
            1.0f - depth * level;

        //==============================================================
        // Sag envelope
        //
        // Falling envelope = increasing demand -> attack
        // Rising envelope  = decreasing demand -> release

        const float coefficient =
            target < envelope
                ? attackCoefficient
                : releaseCoefficient;

        envelope +=
            coefficient * (target - envelope);
        
        return envelope;
    }

    //==========================================================================
    void process(const float* input,
                 float* output,
                 int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
            output[i] = processSample(input[i]);
    }

    //==========================================================================
    void processBuffer(const float* input,
                       float* envelopeBuffer,
                       int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
            envelopeBuffer[i] = processSample(input[i]);
    }

    //==========================================================================
    float getEnvelope() const
    {
        return envelope;
    }

    //==========================================================================
    float getRms() const
    {
        if (rmsWindowSamples <= 0)
            return 0.0f;

        return std::sqrt(
            static_cast<float>(
                std::max(
                    0.0,
                    rmsSumSquares
                    / static_cast<double>(
                        rmsWindowSamples))));
    }

private:

    //==========================================================================
    void updateRmsWindow()
    {
        if (sampleRate <= 0.0)
            return;

        rmsWindowSamples =
            std::max(
                1,
                static_cast<int>(
                    std::round(
                        sampleRate
                        * rmsWindowMs
                        * 0.001)));

        rmsHistory.assign(
            static_cast<size_t>(rmsWindowSamples),
            0.0f);

        rmsWritePosition = 0;
        rmsSumSquares = 0.0;
    }

    //==========================================================================
    void updateCoefficients()
    {
        if (sampleRate <= 0.0)
            return;

        attackCoefficient =
            1.0f -
            std::exp(
                -1.0f /
                (static_cast<float>(sampleRate)
                 * attackMs
                 * 0.001f));

        releaseCoefficient =
            1.0f -
            std::exp(
                -1.0f /
                (static_cast<float>(sampleRate)
                 * releaseMs
                 * 0.001f));
    }

    //==========================================================================

    double sampleRate = 44100.0;

    // Sag response
    float attackMs  = 5.0f;
    float releaseMs = 80.0f;

    float depth = 0.5f;

    float envelope = 1.0f;

    float attackCoefficient  = 0.0f;
    float releaseCoefficient = 0.0f;

    // RMS detector
    float rmsWindowMs = 8.0f;

    int rmsWindowSamples = 352;
    int rmsWritePosition = 0;

    std::vector<float> rmsHistory;

    double rmsSumSquares = 0.0;
};
