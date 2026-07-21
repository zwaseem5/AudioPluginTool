"""Thin wrapper around the compiled RenderHarness C++ tool.

RenderHarness renders a known test signal (white noise or an impulse)
through the plugin's real AudioProcessor -- the same gain -> low-pass ->
high-pass -> tremolo -> clipper chain used in the actual plugin -- and
writes the result to a WAV file. This module invokes that binary and loads
the result as a numpy array so the Python tests can analyse it.
"""

import subprocess
from pathlib import Path

import soundfile as sf

REPO_ROOT = Path(__file__).resolve().parent.parent
HARNESS_PATH = REPO_ROOT / "build" / "RenderHarness"
TMP_DIR = REPO_ROOT / "tests" / "tmp"


def render(output_name, **params):
    """Render a test signal through the plugin and return (audio, sample_rate).

    audio is a numpy array of shape (num_samples, num_channels).
    Keyword args map to RenderHarness CLI flags, e.g. lowpass=1000 becomes
    --lowpass=1000, tremolo_rate=5 becomes --tremolo-rate=5.
    """
    if not HARNESS_PATH.exists():
        raise FileNotFoundError(
            f"RenderHarness not found at {HARNESS_PATH}. "
            f"Build it first: cmake --build build --target RenderHarness"
        )

    TMP_DIR.mkdir(parents=True, exist_ok=True)
    output_path = TMP_DIR / output_name

    args = [str(HARNESS_PATH), f"--output={output_path}"]
    for key, value in params.items():
        cli_key = key.replace("_", "-")
        args.append(f"--{cli_key}={value}")

    result = subprocess.run(args, capture_output=True, text=True)
    if result.returncode != 0:
        raise RuntimeError(
            f"RenderHarness failed (exit {result.returncode}):\n"
            f"stdout: {result.stdout}\nstderr: {result.stderr}"
        )

    audio, sample_rate = sf.read(str(output_path), dtype="float32", always_2d=True)
    return audio, sample_rate
