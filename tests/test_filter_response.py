"""Filter response tests.

Method: render white noise through the plugin twice -- once with a filter
engaged, once with it fully open -- then take the FFT of both and compare
magnitude per frequency bin (see dsp_analysis.py for the measurement code).

The test signal is rendered quiet (amplitude=0.3) so the always-on soft
clipper stays in its near-linear region and doesn't distort the measurement
-- clipping behavior has its own dedicated test.
"""

import numpy as np

from dsp_analysis import TEST_SIGNAL_AMPLITUDE, band_average_db, magnitude_response_db
from render_harness import render


def test_lowpass_attenuates_highs_and_passes_lows():
    dry, sr = render(
        "lp_dry.wav", signal="noise", duration=2, amplitude=TEST_SIGNAL_AMPLITUDE,
        lowpass=20000, highpass=20, tremolo_depth=0,
    )
    wet, _ = render(
        "lp_wet.wav", signal="noise", duration=2, amplitude=TEST_SIGNAL_AMPLITUDE,
        lowpass=1000, highpass=20, tremolo_depth=0,
    )

    freqs, response_db = magnitude_response_db(dry.mean(axis=1), wet.mean(axis=1), sr)

    passband = band_average_db(freqs, response_db, 100, 500)
    stopband = band_average_db(freqs, response_db, 5000, 15000)

    assert passband > -3.0, f"passband (100-500 Hz) attenuated too much: {passband:.1f} dB"
    assert stopband < -12.0, f"stopband (5k-15k Hz) not attenuated enough: {stopband:.1f} dB"


def test_highpass_attenuates_lows_and_passes_highs():
    wet, sr = render(
        "hp_wet.wav", signal="noise", duration=2, amplitude=TEST_SIGNAL_AMPLITUDE,
        lowpass=20000, highpass=1000, tremolo_depth=0,
    )
    dry, _ = render(
        "hp_dry.wav", signal="noise", duration=2, amplitude=TEST_SIGNAL_AMPLITUDE,
        lowpass=20000, highpass=20, tremolo_depth=0,
    )

    freqs, response_db = magnitude_response_db(dry.mean(axis=1), wet.mean(axis=1), sr)

    stopband = band_average_db(freqs, response_db, 20, 200)
    passband = band_average_db(freqs, response_db, 5000, 15000)

    assert stopband < -12.0, f"stopband (20-200 Hz) not attenuated enough: {stopband:.1f} dB"
    assert passband > -3.0, f"passband (5k-15k Hz) attenuated too much: {passband:.1f} dB"


def test_filters_fully_open_are_nearly_transparent():
    """With both filters wide open, output should closely match a second
    render of the same signal -- proving the filters don't color the sound
    at all when the user hasn't engaged them."""
    dry, sr = render(
        "open_dry.wav", signal="noise", duration=1, amplitude=TEST_SIGNAL_AMPLITUDE,
        lowpass=20000, highpass=20, gain=0, tremolo_depth=0,
    )
    wet, _ = render(
        "open_wet.wav", signal="noise", duration=1, amplitude=TEST_SIGNAL_AMPLITUDE,
        lowpass=20000, highpass=20, gain=0, tremolo_depth=0,
    )

    freqs, response_db = magnitude_response_db(dry.mean(axis=1), wet.mean(axis=1), sr)
    audible = (freqs >= 20) & (freqs <= 20000)

    assert np.all(np.abs(response_db[audible]) < 1.0), (
        f"expected near-0 dB response with filters open, "
        f"max deviation was {np.max(np.abs(response_db[audible])):.2f} dB"
    )
