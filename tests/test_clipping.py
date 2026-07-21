"""Clipping protection tests.

The plugin's final stage is a soft (tanh-based) clipper that mathematically
guarantees output samples never exceed +-1.0 (0 dBFS), no matter how much
gain is applied upstream. These tests verify that guarantee holds under
extreme gain, that the clipper is actually being exercised (not just
trivially passing because nothing loud enough was sent through), and that
the saturation is smooth rather than the flat-topped shape of hard clipping.
"""

import numpy as np

from render_harness import render


def test_output_never_exceeds_full_scale_under_extreme_gain():
    audio, sr = render(
        "clip_extreme.wav", signal="noise", duration=2, amplitude=1.0,
        gain=24, lowpass=20000, highpass=20, tremolo_depth=0,
    )
    peak = float(np.max(np.abs(audio)))
    assert peak <= 1.0 + 1e-6, f"output exceeded 0 dBFS: peak={peak}"


def test_clipping_actually_engages_at_high_gain():
    audio, sr = render(
        "clip_engage.wav", signal="noise", duration=2, amplitude=1.0,
        gain=24, lowpass=20000, highpass=20, tremolo_depth=0,
    )
    fraction_near_ceiling = float(np.mean(np.abs(audio) > 0.9))
    assert fraction_near_ceiling > 0.05, (
        f"expected the clipper to be meaningfully engaged at +24 dB gain, "
        f"only {fraction_near_ceiling:.1%} of samples were near the ceiling"
    )


def test_clipping_is_smooth_not_hard():
    """A hard clipper produces long runs of samples pinned at exactly the
    same value (a flat-topped waveform). Our soft clipper should never do
    this -- tanh() approaches but never touches +-1.0.

    Note: this test deliberately uses a moderate gain (+6 dB), not the
    extreme +24 dB used above. At +24 dB, input samples reach magnitudes
    where tanh(x) is mathematically still smooth but rounds to the exact
    float32 value 1.0 (32-bit float only has ~7 significant digits), which
    would make even genuine soft clipping look "pinned" here. +6 dB keeps
    inputs in a range where the curve's shape is still visible in float32.
    """
    audio, sr = render(
        "clip_smooth.wav", signal="noise", duration=1, amplitude=1.0,
        gain=6, lowpass=20000, highpass=20, tremolo_depth=0,
    )

    exactly_at_ceiling = float(np.mean(np.abs(audio) >= 0.999999))
    assert exactly_at_ceiling < 0.001, (
        f"too many samples pinned exactly at the ceiling ({exactly_at_ceiling:.4%}) "
        f"-- this looks like hard clipping, not soft saturation"
    )

    in_the_shoulder = float(np.mean((np.abs(audio) > 0.85) & (np.abs(audio) < 0.98)))
    assert in_the_shoulder > 0.01, (
        f"expected a meaningful fraction of samples in the smooth saturation "
        f"'shoulder' (0.85-0.98), found only {in_the_shoulder:.2%} -- "
        f"the curve may not be engaging as expected"
    )
