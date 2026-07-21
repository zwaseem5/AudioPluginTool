"""Output consistency (determinism) tests.

Running the exact same input through the exact same parameters should
produce bit-for-bit identical output every time. This matters for
regression testing: if a future code change makes the DSP path
nondeterministic (an uninitialized variable, a data race, reliance on
system time, etc), these tests catch it immediately.
"""

import numpy as np

from render_harness import render


def test_identical_input_and_params_produce_identical_output():
    audio1, sr1 = render(
        "consistency_1.wav", signal="noise", duration=2, seed=123,
        gain=6, lowpass=4000, highpass=100, tremolo_rate=3, tremolo_depth=40,
    )
    audio2, sr2 = render(
        "consistency_2.wav", signal="noise", duration=2, seed=123,
        gain=6, lowpass=4000, highpass=100, tremolo_rate=3, tremolo_depth=40,
    )

    assert sr1 == sr2
    assert np.array_equal(audio1, audio2), "identical inputs produced different output"


def test_different_seeds_produce_different_output():
    """Sanity check for the test above: if this ever fails, our noise
    generator isn't actually using the seed, which would make the
    determinism test meaningless (it'd pass even if renders were random)."""
    audio1, _ = render("consistency_seedA.wav", signal="noise", duration=1, seed=1)
    audio2, _ = render("consistency_seedB.wav", signal="noise", duration=1, seed=2)

    assert not np.array_equal(audio1, audio2)
