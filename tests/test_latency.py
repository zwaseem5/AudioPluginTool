"""Latency test.

Feeds a single-sample impulse through the plugin (filters fully open, so
we're measuring the architecture's own latency, not a filter's group
delay) and finds where the peak lands in the output. Our chain processes
samples in-place with no lookahead, buffering, or oversampling, so it
should introduce close to zero samples of latency -- unlike plugins that
use FFT-based processing or oversampling, which report nonzero latency to
the host via getLatencySamples().
"""

import numpy as np

from render_harness import render


def test_impulse_latency_is_near_zero():
    audio, sr = render(
        "latency_impulse.wav", signal="impulse", duration=0.5, amplitude=1.0,
        gain=0, lowpass=20000, highpass=20, tremolo_depth=0,
    )
    mono = audio.mean(axis=1)
    peak_sample = int(np.argmax(np.abs(mono)))
    latency_ms = (peak_sample / sr) * 1000.0

    assert peak_sample <= 4, (
        f"expected the impulse peak within a few samples of the input, "
        f"found it at sample {peak_sample} ({latency_ms:.3f} ms)"
    )
