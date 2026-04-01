#include "PitchEngine.h"

#include <cmath>
#include <algorithm>

void PitchEngine::prepare(double sampleRate, int samplesPerBlock)
{
    sampleRate_      = sampleRate;
    samplesPerBlock_ = samplesPerBlock;
    smoothedPitchHz  = 0.0f;
    pitchHistory.fill(0.0f);
    historyIndex = 0;
    lastStableHz = 0.0f;
}

void PitchEngine::setVocalRange(float minHz, float maxHz)
{
    vocalMinHz_ = minHz;
    vocalMaxHz_ = maxHz;
    lastStableHz = 0.0f; // reset continuity so first post-calibration note isn't biased
}

float PitchEngine::median(std::array<float, kHistorySize> h)
{
    std::sort(h.begin(), h.end());
    return h[kHistorySize / 2];
}

float PitchEngine::nearestOctave(float hz, float lastHz, float minHz, float maxHz)
{
    if (hz <= 0.0f || lastHz <= 0.0f)
        return hz;

    const float candidates[] = { hz * 0.5f, hz, hz * 2.0f };
    float best     = hz;
    float bestDist = std::abs(1200.0f * std::log2(hz / lastHz));

    for (float c : candidates)
    {
        if (c < minHz || c > maxHz)
            continue;
        const float dist = std::abs(1200.0f * std::log2(c / lastHz));
        if (dist < bestDist)
        {
            bestDist = dist;
            best     = c;
        }
    }
    return best;
}

float PitchEngine::process(const float* buf, int bufSize, int writeIndex,
                           float sensitivity, float rms, float noiseGateDb, float smoothingMs)
{
    // Noise gate: block detection entirely when signal is too quiet.
    const float gateLinear = std::pow(10.0f, noiseGateDb / 20.0f);
    const float raw = (rms >= gateLinear)
                          ? detectPitch(buf, bufSize, writeIndex, sampleRate_, sensitivity,
                                        vocalMinHz_, vocalMaxHz_)
                          : 0.0f;

    pitchHistory[historyIndex] = raw;
    historyIndex = (historyIndex + 1) % kHistorySize;

    const float hz = nearestOctave(median(pitchHistory), lastStableHz,
                                   vocalMinHz_, vocalMaxHz_);
    lastStableHz = hz;

    // EMA smoothing for display; alpha is recomputed each block so the param
    // takes effect immediately without requiring a prepare() call.
    const float alpha = (smoothingMs <= 0.0f)
                            ? 1.0f
                            : 1.0f - std::exp(-static_cast<float>(samplesPerBlock_)
                                              / (static_cast<float>(sampleRate_) * smoothingMs / 1000.0f));
    smoothedPitchHz = (hz > 0.0f)
                          ? alpha * hz + (1.0f - alpha) * smoothedPitchHz
                          : 0.0f;

    return hz;
}

float PitchEngine::detectPitch(const float* buf, int bufSize, int writeIndex,
                               double sampleRate, float sensitivity,
                               float minHz, float maxHz)
{
    const int minLag = static_cast<int>(sampleRate / static_cast<double>(maxHz));
    const int maxLag = std::min(static_cast<int>(sampleRate / static_cast<double>(minHz)),
                                bufSize - 1);

    if (minLag >= maxLag)
        return 0.0f;

    // Autocorrelation at lag 0 == signal energy; gate on minimum level
    float corr0 = 0.0f;
    for (int i = 0; i < bufSize; ++i)
        corr0 += buf[i] * buf[i];

    const float rmsThreshold = std::pow(10.0f, -6.0f + sensitivity * 4.0f);
    if (corr0 / static_cast<float>(bufSize) < rmsThreshold)
        return 0.0f;

    // Find lag with highest autocorrelation in the search range
    float bestCorr = 0.0f;
    int   bestLag  = 0;

    for (int lag = minLag; lag <= maxLag; ++lag)
    {
        float corr = 0.0f;
        for (int i = 0; i < bufSize - lag; ++i)
        {
            const int idxA = (writeIndex + i)       & (bufSize - 1);
            const int idxB = (writeIndex + i + lag) & (bufSize - 1);
            corr += buf[idxA] * buf[idxB];
        }
        if (corr > bestCorr)
        {
            bestCorr = corr;
            bestLag  = lag;
        }
    }

    // Require the best correlation to be at least 30 % of the zero-lag energy
    if (bestLag == 0 || bestCorr < 0.30f * corr0)
        return 0.0f;

    // Octave-error correction: if half the best lag is also in range and its
    // correlation is >= 85 % of the best, prefer it (true pitch is one octave up).
    // Repeat once more to catch double-octave errors.
    for (int pass = 0; pass < 2; ++pass)
    {
        const int halfLag = bestLag / 2;
        if (halfLag < minLag)
            break;
        float halfCorr = 0.0f;
        for (int i = 0; i < bufSize - halfLag; ++i)
        {
            const int idxA = (writeIndex + i)           & (bufSize - 1);
            const int idxB = (writeIndex + i + halfLag) & (bufSize - 1);
            halfCorr += buf[idxA] * buf[idxB];
        }
        if (halfCorr >= 0.85f * bestCorr)
        {
            bestLag  = halfLag;
            bestCorr = halfCorr;
        }
        else
            break;
    }

    return static_cast<float>(sampleRate / static_cast<double>(bestLag));
}
