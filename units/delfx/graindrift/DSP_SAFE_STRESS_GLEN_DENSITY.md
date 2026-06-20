# DSP_SAFE_STRESS_GLEN_DENSITY

Hardware stress test for GrainDrift / GrainLab Phase 1 stability.

## Purpose

Verify that the known high-load setting does not stop the drumlogue audio engine.
If load becomes too high, GrainDrift must drop new grains instead of exceeding
the fixed grain pool.

## Settings

- Slot: DelayFX
- Source: short percussive sequence, e.g. VPM synth or short drum hits
- Delay send: above 50 percent
- DENSITY: 96 percent
- RHYTHM: 1/16
- PITCH: -12
- G-LEN: sweep from 60 percent to 100 percent
- CHAOS: low to medium
- COLOR: CLEAN first, then LOFI

## Acceptance

- No DSP or audio-engine dropout.
- No complete sound loss.
- No power cycle required.
- Glitchy audio is acceptable.
- Grain dropping is acceptable under overload.
- Output remains bounded by the internal limiter.

## Phase 1 Rules

- Maximum active grains: 16.
- No dynamic allocation in the audio callback.
- Full grain pool means drop new grain.
- No new creative V2 features are part of this test.
