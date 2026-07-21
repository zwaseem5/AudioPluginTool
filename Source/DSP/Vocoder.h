#pragma once

#include <juce_dsp/juce_dsp.h>
#include <array>
#include <vector>
#include "PitchTracker.h"

// Classic vocoder ("robot voice") effect. Splits the input ("modulator",
// e.g. a vocal) and an internally generated sawtooth tone ("carrier") into
// the same set of frequency bands, tracks the modulator's volume envelope
// in each band, then applies that envelope to the matching carrier band.
// Summing the shaped carrier bands back together imposes the vocal's
// rhythm/formants onto the carrier's pitch -- the source of the effect's
// robotic, synthetic-speech character.
//
// The carrier can either sit at a fixed pitch (classic monotone "robot
// voice") or track the pitch of whatever you're singing, via PitchTracker
// -- the latter is what makes the effect feel "playable" with a sung
// melody instead of a static drone, closer to how a vocoder sounds musical
// in a mix rather than just novelty-robotic.
class Vocoder
{
public:
    // 16 bands is a solid middle ground for a "professional-ish" vocoder:
    // enough spectral resolution to keep speech intelligible, without the
    // CPU cost of the 20+ bands high-end hardware vocoders use.
    static constexpr size_t numBands = 16;

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
        setCarrierFrequency (carrierFrequencyHz);
        pitchTracker.prepare (spec.sampleRate);

        channels.clear();
        channels.resize (spec.numChannels);

        juce::dsp::ProcessSpec bandSpec { spec.sampleRate, spec.maximumBlockSize, 1 };

        for (auto& channel : channels)
        {
            for (size_t band = 0; band < numBands; ++band)
            {
                auto centreHz = bandCentreFrequency (band);
                auto coeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass (sampleRate, centreHz, bandQ);

                channel.modulatorFilters[band].coefficients = coeffs;
                channel.modulatorFilters[band].prepare (bandSpec);

                // A separate (but equal) coefficients object per filter --
                // sharing one Coefficients::Ptr between the modulator and
                // carrier filter for a band would be fine too (coefficients
                // are read-only once assigned), but keeping them distinct
                // avoids any confusion about shared state.
                channel.carrierFilters[band].coefficients =
                    juce::dsp::IIR::Coefficients<float>::makeBandPass (sampleRate, centreHz, bandQ);
                channel.carrierFilters[band].prepare (bandSpec);
            }
        }

        // Fast attack so the envelope catches the start of a syllable
        // quickly; slower release so it smooths out the waveform's own
        // ripple instead of tracking every individual cycle (which is what
        // produced the "humming" instead of following actual speech).
        attackCoeff = std::exp (-1.0f / (0.003f * (float) sampleRate));
        releaseCoeff = std::exp (-1.0f / (0.030f * (float) sampleRate));
    }

    void reset()
    {
        phase = 0.0f;
        phase2 = 0.0f;
        pitchTracker.reset();

        for (auto& channel : channels)
        {
            for (size_t band = 0; band < numBands; ++band)
            {
                channel.modulatorFilters[band].reset();
                channel.carrierFilters[band].reset();
                channel.envelopes[band] = 0.0f;
            }
        }
    }

    void setCarrierFrequency (float newFrequencyHz)
    {
        carrierFrequencyHz = newFrequencyHz;
        phaseIncrement = juce::MathConstants<float>::twoPi * carrierFrequencyHz / (float) sampleRate;
    }

    void setMix (float newMix01)
    {
        mix = juce::jlimit (0.0f, 1.0f, newMix01);
    }

    void setPitchTrackingEnabled (bool shouldTrackPitch)
    {
        pitchTrackingEnabled = shouldTrackPitch;
    }

    void setNoteSnapEnabled (bool shouldSnapToNotes)
    {
        noteSnapEnabled = shouldSnapToNotes;
    }

    // rootNote: pitch class 0-11 (0 = C, 1 = C#, ... 11 = B).
    void setMusicalKey (int rootNote, bool useMinorScale)
    {
        keyRootNote = rootNote;
        keyIsMinor = useMinorScale;
    }

    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
        const auto& inputBlock = context.getInputBlock();
        auto& outputBlock = context.getOutputBlock();
        const auto numChannels = outputBlock.getNumChannels();
        const auto numSamples = outputBlock.getNumSamples();

        if (context.isBypassed || mix <= 0.0f)
        {
            outputBlock.copyFrom (inputBlock);
            return;
        }

        if (pitchTrackingEnabled && numChannels > 0)
        {
            pitchTracker.pushSamples (inputBlock.getChannelPointer (0), (int) numSamples);
            auto trackedHz = pitchTracker.getFrequencyHz();
            setCarrierFrequency (noteSnapEnabled ? quantizeToScale (trackedHz, keyRootNote, keyIsMinor) : trackedHz);
        }

        for (size_t sample = 0; sample < numSamples; ++sample)
        {
            // One shared pair of carrier phases drives every channel, so a
            // stereo signal stays phase-locked instead of the channels'
            // carriers drifting apart. Two sawtooths a few cents apart
            // (rather than one) give the carrier a thicker, "synth pad"
            // quality instead of a thin single tone -- the same cheap
            // detuning trick behind most lush analog-style synth sounds.
            auto saw1 = (phase / juce::MathConstants<float>::pi) - 1.0f;
            auto saw2 = (phase2 / juce::MathConstants<float>::pi) - 1.0f;
            auto carrierSample = 0.5f * (saw1 + saw2);

            phase += phaseIncrement;
            if (phase >= juce::MathConstants<float>::twoPi)
                phase -= juce::MathConstants<float>::twoPi;

            phase2 += phaseIncrement * detuneRatio;
            if (phase2 >= juce::MathConstants<float>::twoPi)
                phase2 -= juce::MathConstants<float>::twoPi;

            for (size_t ch = 0; ch < numChannels; ++ch)
            {
                auto& channel = channels[ch];
                auto dry = inputBlock.getChannelPointer (ch)[sample];

                float vocodedSample = 0.0f;

                for (size_t band = 0; band < numBands; ++band)
                {
                    auto modBand = channel.modulatorFilters[band].processSample (dry);
                    auto carrierBand = channel.carrierFilters[band].processSample (carrierSample);

                    auto rectified = std::abs (modBand);
                    auto& envelope = channel.envelopes[band];
                    auto coeff = rectified > envelope ? attackCoeff : releaseCoeff;
                    envelope = coeff * envelope + (1.0f - coeff) * rectified;

                    vocodedSample += carrierBand * envelope;
                }

                vocodedSample *= bandMakeupGain;

                outputBlock.getChannelPointer (ch)[sample] = dry * (1.0f - mix) + vocodedSample * mix;
            }
        }
    }

private:
    using BandFilter = juce::dsp::IIR::Filter<float>;

    struct ChannelBands
    {
        std::array<BandFilter, numBands> modulatorFilters;
        std::array<BandFilter, numBands> carrierFilters;
        std::array<float, numBands> envelopes {};
    };

    // Snaps a detected pitch to the nearest note that belongs to the given
    // musical key -- the same principle behind Auto-Tune-style pitch
    // correction, but scale-aware rather than purely chromatic. Singing
    // slightly off pitch still comes out as a clean, in-tune note that
    // actually belongs to the song's key, instead of just the nearest of
    // all 12 semitones (which can snap to a note that clashes with the
    // music). The discrete jumps between notes as you slide between
    // pitches are what give quantized vocoder/talkbox vocals their tight,
    // robotic character.
    static float quantizeToScale (float hz, int rootNote, bool useMinorScale)
    {
        if (hz <= 0.0f)
            return hz;

        static constexpr std::array<int, 7> majorIntervals { 0, 2, 4, 5, 7, 9, 11 };
        static constexpr std::array<int, 7> minorIntervals { 0, 2, 3, 5, 7, 8, 10 };
        const auto& intervals = useMinorScale ? minorIntervals : majorIntervals;

        auto midiNote = 69.0f + 12.0f * std::log2 (hz / 440.0f); // A4 = MIDI 69 = 440 Hz
        auto roundedNote = (int) std::round (midiNote);

        int bestNote = roundedNote;
        int bestDistance = 1000;

        // Search +-1 octave around the raw detected note for the closest
        // in-scale note, rather than assuming the answer lies within the
        // same octave (the nearest scale tone might be just across an
        // octave boundary).
        for (int candidate = roundedNote - 12; candidate <= roundedNote + 12; ++candidate)
        {
            auto pitchClass = ((candidate - rootNote) % 12 + 12) % 12;

            bool inScale = false;
            for (auto interval : intervals)
                if (interval == pitchClass) { inScale = true; break; }

            if (inScale)
            {
                auto distance = std::abs (candidate - roundedNote);
                if (distance < bestDistance)
                {
                    bestDistance = distance;
                    bestNote = candidate;
                }
            }
        }

        return 440.0f * std::pow (2.0f, ((float) bestNote - 69.0f) / 12.0f);
    }

    static float bandCentreFrequency (size_t bandIndex)
    {
        // Log-spaced bands from 100 Hz to 8 kHz -- covers the fundamental,
        // vocal formants, and enough high-frequency content for sibilance
        // ("s", "sh", "f" sounds) to still read as intelligible.
        constexpr float lowHz = 100.0f;
        constexpr float highHz = 8000.0f;
        auto t = (float) bandIndex / (float) (numBands - 1);
        return lowHz * std::pow (highHz / lowHz, t);
    }

    std::vector<ChannelBands> channels;
    PitchTracker pitchTracker;

    double sampleRate = 44100.0;
    float phase = 0.0f;
    float phase2 = 0.0f;
    float phaseIncrement = 0.0f;
    float carrierFrequencyHz = 110.0f;
    float mix = 0.0f;
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;
    bool pitchTrackingEnabled = false;
    bool noteSnapEnabled = false;
    int keyRootNote = 0; // 0 = C
    bool keyIsMinor = false;

    // Q ~2.5 keeps each band narrow enough for reasonable spectral
    // separation without becoming so resonant it "rings" at its own
    // frequency almost independent of the input -- that self-ringing is
    // what caused the earlier version to sound like a hum instead of
    // tracking actual speech content.
    static constexpr float bandQ = 2.5f;
    static constexpr float bandMakeupGain = 3.0f;

    // ~10 cents of detune between the two carrier saws -- enough to sound
    // thick/lush, subtle enough to still read as one pitch, not two.
    static constexpr float detuneRatio = 1.006f;
};
