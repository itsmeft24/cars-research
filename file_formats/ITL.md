# DSP / ITL — Interleaved Audio

Not a full format spec, just the channel layout used by the audio helper [`tools/dsp2itl.py`](../tools/dsp2itl.py).

- Inputs are two mono Nintendo **DSP (GC ADPCM)** files — left and right channel.
- Output (`.itl`) is the same data **interleaved in `0x10000`-byte blocks**: `L[0], R[0], L[1], R[1], …`.
- Trailing data that doesn't fill a whole block follows the last full pair: the remaining left bytes, then the remaining right bytes.
- No header is added, so the DSP headers inside the channel data are copied as-is.

The script doesn't validate that both channels have equal length, and it uses `r+b` modes, so work on copies.
