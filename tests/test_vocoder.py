"""Vocoder tests.

The vocoder splits the input into frequency bands, tracks each band's
volume envelope, and imposes that envelope onto an internally generated
carrier tone. A broken implementation (e.g. band filters resonant enough to
self-oscillate) can produce a near-constant hum that barely reacts to the
input at all -- these tests catch exactly that failure mode by feeding a
signal with a known on/off rhythm and checking the vocoded output's
loudness actually follows it.
"""

import numpy as np

from render_harness import render


def rms(samples):
    return float(np.sqrt(np.mean(samples.astype(np.float64) ** 2)))


def test_vocoder_output_tracks_input_rhythm():
    """Feed alternating 200ms noise / 200ms silence ('speech-like' bursts)
    with the vocoder fully wet, and confirm the output is meaningfully
    louder during the 'on' windows than the 'off' windows -- proving the
    effect follows the input's dynamics rather than humming constantly."""
    audio, sr = render(
        "vocoder_bursts.wav", signal="bursts", duration=3.0, amplitude=0.8,
        gain=0, lowpass=20000, highpass=20, tremolo_depth=0,
        vocoder_mix=100, vocoder_carrier_hz=110, vocoder_pitch_track=0,
    )
    mono = audio.mean(axis=1)

    cycle_samples = int(0.2 * sr)
    on_samples = cycle_samples // 2

    # The envelope follower's attack (~3ms) settles fast, but its release
    # (~30ms) needs roughly 3x its time constant (~90ms) to fully decay --
    # skip past that ringing tail before measuring the "off" window, or
    # this test would be measuring decay-in-progress, not steady-state.
    on_margin = int(0.01 * sr)
    off_margin = int(0.09 * sr)

    on_chunks, off_chunks = [], []
    for start in range(0, len(mono) - cycle_samples, cycle_samples):
        on_chunks.append(mono[start + on_margin: start + on_samples - on_margin])
        off_chunks.append(mono[start + on_samples + off_margin: start + cycle_samples])

    on_level = rms(np.concatenate(on_chunks))
    off_level = rms(np.concatenate(off_chunks))

    assert on_level > 1e-6, "vocoder produced no output at all during 'on' windows"

    ratio_db = 20 * np.log10(on_level / max(off_level, 1e-8))
    assert ratio_db > 15.0, (
        f"vocoder output doesn't track input rhythm: on={on_level:.4f}, "
        f"off={off_level:.4f} ({ratio_db:.1f} dB difference, expected > 15 dB) "
        f"-- this is the 'constant humming' failure mode."
    )


def test_vocoder_stays_quiet_with_silent_input():
    """Even with the vocoder fully wet, silent input should produce silent
    output. A self-oscillating filter bank would keep humming regardless."""
    audio, sr = render(
        "vocoder_silence.wav", signal="noise", duration=1.0, amplitude=0.0,
        gain=0, lowpass=20000, highpass=20, tremolo_depth=0,
        vocoder_mix=100, vocoder_carrier_hz=110, vocoder_pitch_track=0,
    )
    peak = float(np.max(np.abs(audio)))
    assert peak < 0.01, f"expected near-silence with silent input, got peak={peak:.4f}"


def test_vocoder_pitch_tracking_follows_sung_note():
    """With pitch tracking on, the carrier should lock onto whatever pitch
    is being 'sung' -- simulated here with a clean sine tone standing in
    for a sustained sung note -- rather than sitting at the manually-set
    carrier frequency. Render two different tones and confirm the vocoded
    output's dominant frequency shifts to follow each one."""
    for tone_hz in (220.0, 330.0):
        audio, sr = render(
            f"vocoder_pitch_track_{int(tone_hz)}.wav", signal="tone", duration=1.5,
            amplitude=0.5, tone_hz=tone_hz, gain=0, lowpass=20000, highpass=20,
            tremolo_depth=0, vocoder_mix=100, vocoder_carrier_hz=110, vocoder_pitch_track=1,
        )
        mono = audio.mean(axis=1)

        # Skip the first third of a second: the pitch tracker needs a few
        # analysis windows (each ~30ms) to converge on the sung note.
        settle_samples = int(0.3 * sr)
        steady_state = mono[settle_samples:]

        spectrum = np.abs(np.fft.rfft(steady_state * np.hanning(len(steady_state))))
        freqs = np.fft.rfftfreq(len(steady_state), d=1.0 / sr)

        peak_freq = float(freqs[np.argmax(spectrum)])

        assert abs(peak_freq - tone_hz) < 15.0, (
            f"expected the vocoder's carrier to track a {tone_hz} Hz sung note, "
            f"but the output's dominant frequency was {peak_freq:.1f} Hz"
        )


def test_vocoder_note_snap_quantizes_to_nearest_semitone():
    """Sing slightly off-pitch (simulated: a 225 Hz tone, between A3=220 Hz
    and A#3=233.08 Hz but closer to A3) and confirm that with note-snap on,
    the carrier locks to the exact in-tune frequency (220 Hz) rather than
    the raw, slightly-flat tracked pitch."""
    snapped, sr = render(
        "vocoder_snap_on.wav", signal="tone", duration=1.5, amplitude=0.5,
        tone_hz=225.0, gain=0, lowpass=20000, highpass=20, tremolo_depth=0,
        vocoder_mix=100, vocoder_pitch_track=1, vocoder_note_snap=1,
    )
    unsnapped, _ = render(
        "vocoder_snap_off.wav", signal="tone", duration=1.5, amplitude=0.5,
        tone_hz=225.0, gain=0, lowpass=20000, highpass=20, tremolo_depth=0,
        vocoder_mix=100, vocoder_pitch_track=1, vocoder_note_snap=0,
    )

    settle_samples = int(0.3 * sr)

    def dominant_freq(audio):
        steady_state = audio.mean(axis=1)[settle_samples:]
        spectrum = np.abs(np.fft.rfft(steady_state * np.hanning(len(steady_state))))
        freqs = np.fft.rfftfreq(len(steady_state), d=1.0 / sr)
        return float(freqs[np.argmax(spectrum)])

    snapped_freq = dominant_freq(snapped)
    unsnapped_freq = dominant_freq(unsnapped)

    assert abs(snapped_freq - 220.0) < 5.0, (
        f"expected note-snap to quantize 225 Hz to the nearest note (A3, 220 Hz), "
        f"got {snapped_freq:.1f} Hz"
    )
    assert abs(unsnapped_freq - 225.0) < 15.0, (
        f"expected raw (unsnapped) tracking to stay near the sung 225 Hz, "
        f"got {unsnapped_freq:.1f} Hz -- if this fails, the snap-off control "
        f"itself may not be working"
    )


def test_vocoder_note_snap_respects_musical_key():
    """A 277 Hz tone sits right at C#, which isn't in the C major scale but
    is (trivially) in C# major. With key=C major it should snap away to a
    neighboring in-key note; with key=C# major it should stay close to its
    own pitch. This proves the Key selector actually changes which notes
    count as 'in tune', not just generic nearest-semitone snapping."""
    common = dict(
        signal="tone", duration=1.5, amplitude=0.5, tone_hz=277.0,
        gain=0, lowpass=20000, highpass=20, tremolo_depth=0,
        vocoder_mix=100, vocoder_pitch_track=1, vocoder_note_snap=1, vocoder_scale=0,
    )

    in_c, sr = render("vocoder_key_c.wav", vocoder_key=0, **common)        # C major
    in_csharp, _ = render("vocoder_key_csharp.wav", vocoder_key=1, **common)  # C# major

    settle_samples = int(0.3 * sr)

    def dominant_freq(audio):
        steady_state = audio.mean(axis=1)[settle_samples:]
        spectrum = np.abs(np.fft.rfft(steady_state * np.hanning(len(steady_state))))
        freqs = np.fft.rfftfreq(len(steady_state), d=1.0 / sr)
        return float(freqs[np.argmax(spectrum)])

    freq_c = dominant_freq(in_c)
    freq_csharp = dominant_freq(in_csharp)

    assert abs(freq_csharp - 277.0) < 8.0, (
        f"C# is in-key for C# major, expected the output to stay near 277 Hz, "
        f"got {freq_csharp:.1f} Hz"
    )
    assert abs(freq_c - 277.0) > 8.0, (
        f"C# is NOT in-key for C major, expected the output to snap away from "
        f"277 Hz to a neighboring in-key note, got {freq_c:.1f} Hz"
    )


def test_vocoder_is_transparent_at_zero_mix():
    dry, sr = render(
        "vocoder_dry.wav", signal="noise", duration=1.0, amplitude=0.3,
        gain=0, lowpass=20000, highpass=20, tremolo_depth=0, vocoder_mix=0,
    )
    wet, _ = render(
        "vocoder_mix0.wav", signal="noise", duration=1.0, amplitude=0.3,
        gain=0, lowpass=20000, highpass=20, tremolo_depth=0,
        vocoder_mix=0, vocoder_carrier_hz=250,
    )
    assert np.allclose(dry, wet, atol=1e-6)
