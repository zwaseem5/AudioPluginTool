"""Shared FFT-based analysis helpers used by multiple test modules.

Method: given a "dry" (unprocessed) and "wet" (processed) render of the same
white-noise signal, the ratio |wet_fft| / |dry_fft| per frequency bin
approximates a filter's actual frequency response, since white noise has
roughly equal energy at every frequency. This is a standard measurement
technique, not something specific to this plugin.
"""

import numpy as np

TEST_SIGNAL_AMPLITUDE = 0.3


def magnitude_response_db(dry, wet, sample_rate):
    """Return (freqs, response_db) estimating a filter's frequency response."""
    n = len(dry)
    window = np.hanning(n)

    dry_fft = np.abs(np.fft.rfft(dry * window))
    wet_fft = np.abs(np.fft.rfft(wet * window))
    freqs = np.fft.rfftfreq(n, d=1.0 / sample_rate)

    dry_fft = np.maximum(dry_fft, 1e-10)
    wet_fft = np.maximum(wet_fft, 1e-10)
    response_db = 20 * np.log10(wet_fft / dry_fft)

    return freqs, response_db


def band_average_db(freqs, response_db, low_hz, high_hz):
    mask = (freqs >= low_hz) & (freqs <= high_hz)
    return float(np.mean(response_db[mask]))
