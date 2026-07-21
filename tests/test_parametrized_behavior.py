"""Parametrized sweeps across input levels, filter settings, and playback
conditions (sample rate).

The other test files each check one representative setting (e.g. a 1000 Hz
low-pass cutoff, or +24 dB gain). These tests instead sweep a range of
values for each, proving the behavior holds generally rather than at one
cherry-picked point.
"""

import numpy as np
import pytest

from dsp_analysis import TEST_SIGNAL_AMPLITUDE, band_average_db, magnitude_response_db
from render_harness import render


@pytest.mark.parametrize("cutoff_hz", [200, 500, 2000, 6000])
def test_lowpass_attenuates_above_cutoff_at_multiple_settings(cutoff_hz):
    dry, sr = render(
        f"param_lp_dry_{cutoff_hz}.wav", signal="noise", duration=2,
        amplitude=TEST_SIGNAL_AMPLITUDE, lowpass=20000, highpass=20, tremolo_depth=0,
    )
    wet, _ = render(
        f"param_lp_wet_{cutoff_hz}.wav", signal="noise", duration=2,
        amplitude=TEST_SIGNAL_AMPLITUDE, lowpass=cutoff_hz, highpass=20, tremolo_depth=0,
    )

    freqs, response_db = magnitude_response_db(dry.mean(axis=1), wet.mean(axis=1), sr)

    passband = band_average_db(freqs, response_db, cutoff_hz * 0.1, cutoff_hz * 0.3)
    stopband_hi = min(cutoff_hz * 8, sr / 2 - 500)
    stopband = band_average_db(freqs, response_db, cutoff_hz * 3, stopband_hi)

    assert passband > -3.0, f"[{cutoff_hz} Hz cutoff] passband attenuated too much: {passband:.1f} dB"
    assert stopband < -9.0, f"[{cutoff_hz} Hz cutoff] stopband not attenuated enough: {stopband:.1f} dB"


@pytest.mark.parametrize("gain_db", [-12, -6, 0, 6, 12, 18, 24])
def test_output_never_clips_across_input_levels(gain_db):
    """Clipping protection must hold no matter how loud the input is driven."""
    audio, sr = render(
        f"param_gain_{gain_db}.wav", signal="noise", duration=1, amplitude=1.0,
        gain=gain_db, lowpass=20000, highpass=20, tremolo_depth=0,
    )
    peak = float(np.max(np.abs(audio)))
    assert peak <= 1.0 + 1e-6, f"[{gain_db} dB gain] output exceeded 0 dBFS: peak={peak}"


@pytest.mark.parametrize("sample_rate", [44100, 48000, 96000])
def test_filter_behavior_is_consistent_across_sample_rates(sample_rate):
    """Different DAWs and audio interfaces run at different sample rates --
    the filter's behavior relative to its cutoff shouldn't depend on which
    one is active."""
    dry, sr = render(
        f"param_sr_dry_{sample_rate}.wav", signal="noise", duration=2,
        amplitude=TEST_SIGNAL_AMPLITUDE, samplerate=sample_rate,
        lowpass=20000, highpass=20, tremolo_depth=0,
    )
    wet, _ = render(
        f"param_sr_wet_{sample_rate}.wav", signal="noise", duration=2,
        amplitude=TEST_SIGNAL_AMPLITUDE, samplerate=sample_rate,
        lowpass=1000, highpass=20, tremolo_depth=0,
    )

    freqs, response_db = magnitude_response_db(dry.mean(axis=1), wet.mean(axis=1), sr)

    passband = band_average_db(freqs, response_db, 100, 500)
    stopband = band_average_db(freqs, response_db, 5000, 15000)

    assert passband > -3.0, f"[{sample_rate} Hz SR] passband attenuated too much: {passband:.1f} dB"
    assert stopband < -9.0, f"[{sample_rate} Hz SR] stopband not attenuated enough: {stopband:.1f} dB"
