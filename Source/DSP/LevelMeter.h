#pragma once

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

class LevelMeter
{
public:

    //==========================================================================
    void prepare(
        double newSampleRate,
        int newNumChannels,
        float newWindowMs = 50.0f)
    {
        sampleRate = newSampleRate;
        numChannels = std::max(1, newNumChannels);
        windowMs = std::max(1.0f, newWindowMs);

        windowSamples =
            std::max(
                1,
                static_cast<int>(
                    std::round(
                        sampleRate
                        * windowMs
                        * 0.001)));

        // One circular buffer per channel.
        history.assign(
            static_cast<size_t>(numChannels),
            std::vector<float>(
                static_cast<size_t>(windowSamples),
                0.0f));

        sumSquares.assign(
            static_cast<size_t>(numChannels),
            0.0);

        writePosition = 0;
        samplesProcessed = 0;

        currentRms = 0.0f;
        displayedLevel = 0.0f;
        
        updateCoefficients();
    }

    //==========================================================================
    // Process one sample from one channel.
    //
    // The caller must process channels in the same order for every sample
    // block. The circular-buffer position is advanced by processFrame().
    //
    void processSample(
        float sample,
        int channel)
    {
        if (channel < 0 || channel >= numChannels)
            return;

        const float squared = sample * sample;

        auto& channelHistory =
            history[static_cast<size_t>(channel)];

        const float oldSquared =
            channelHistory[
                static_cast<size_t>(writePosition)];

        channelHistory[
            static_cast<size_t>(writePosition)] = squared;
        
        totalSumSquares += squared;
        totalSumSquares -= oldSquared;

        sumSquares[
            static_cast<size_t>(channel)] +=
            static_cast<double>(squared)
            - static_cast<double>(oldSquared);
    }

    //==========================================================================
    // Call once after all channels have supplied the current sample.
    //
    // This advances the common circular-buffer position.
    //
    void processFrame()
    {
        ++samplesProcessed;

        writePosition++;

        if (writePosition >= windowSamples)
            writePosition = 0;

//        const double totalMeanSquare =
//            std::accumulate(
//                sumSquares.begin(),
//                sumSquares.end(),
//                0.0);
//
//        const double meanSquare =
//            totalMeanSquare
//            / (static_cast<double>(windowSamples)
//               * static_cast<double>(numChannels));
        
        const double meanSquare =
            totalSumSquares /
            (windowSamples * numChannels);

        currentRms =
            std::sqrt(
                static_cast<float>(
                    std::max(0.0, meanSquare)));

        updateDisplayedLevel();
    }

    //==========================================================================
    float getLevel() const
    {
        return displayedLevel;
    }

    float getLevelDb() const
    {
        if (displayedLevel <= 0.0f)
            return -120.0f;

        return 20.0f *
               std::log10(displayedLevel);
    }

    //==========================================================================
    void setAttackMs(float milliseconds)
    {
        attackMs = std::max(0.1f, milliseconds);
        updateCoefficients();
    }

    void setReleaseMs(float milliseconds)
    {
        releaseMs = std::max(0.1f, milliseconds);
        updateCoefficients();
    }

    //==========================================================================
    void reset()
    {
        for (auto& channel : history)
            std::fill(channel.begin(), channel.end(), 0.0f);

        std::fill(
            sumSquares.begin(),
            sumSquares.end(),
            0.0);

        writePosition = 0;
        samplesProcessed = 0;

        currentRms = 0.0f;
        displayedLevel = 0.0f;
    }

private:

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

    void updateDisplayedLevel()
    {
        const float coefficient =
            currentRms > displayedLevel
                ? attackCoefficient
                : releaseCoefficient;

        displayedLevel +=
            coefficient *
            (currentRms - displayedLevel);
    }

    //==========================================================================

    double sampleRate = 44100.0;

    int numChannels = 2;

    float windowMs = 50.0f;

    int windowSamples = 2205;

    int writePosition = 0;

    int samplesProcessed = 0;

    std::vector<std::vector<float>> history;

    std::vector<double> sumSquares;
    
    double totalSumSquares = 0.0;

    float currentRms = 0.0f;

    float displayedLevel = 0.0f;

    float attackMs = 5.0f;
    float releaseMs = 200.0f;

    float attackCoefficient = 0.0f;
    float releaseCoefficient = 0.0f;
};
