# Korg Drumlogue Units

Private prototype repository for custom Korg drumlogue units built with the
logue SDK v2.

## Status

- Current units are early prototypes.
- Builds exist, but the units are not yet tested on real drumlogue hardware.
- Use at your own risk.

## Transparency

Parts of this repository were created with AI-assisted prototyping and light
vibecoding support. DSP ideas, source code, and build results are still being
reviewed and refined manually.

## Repository Layout

All units live under one shared path structure, grouped by drumlogue unit type:

```text
units/
  delfx/
    graindrift/
  masterfx/
    drumshuffler/
    mixforge/
    tapevibe/
  revfx/
    gateverb80/
  synth/
```

The local KORG SDK checkout stays separate in `logue-sdk/` and is ignored by
git.

## Units

### DelayFX

- `graindrift`
  Granular stereo delay for ghost grains, pitched echoes, and BPM-synced
  glitch clouds.

### MasterFX

- `mixforge`
  Master bus tool with DJ filter, compressor, saturation, and bitcrush.

- `tapevibe`
  Tape-style color box with drive, wow/flutter, hiss, and dropout behavior.

- `drumshuffler`
  Groove and microtiming processor with tempo-synced offsets and stereo motion.

### ReverbFX

- `gateverb80`
  Gated reverb for classic drum ambience with predelay, tone, width, and wet
  dynamics control.

## Build

Windows helper scripts currently available:

- `build_graindrift.ps1`
- `build_mixforge.ps1`
- `build_tapevibe.ps1`
- `build_drumshuffler.ps1`
- `build_gateverb80.ps1`

Each script syncs one unit into the local SDK build folder and then runs the
official drumlogue Docker build.

Typical deploy targets on the drumlogue:

- `Units/DelayFXs/`
- `Units/MasterFXs/`
- `Units/ReverbFXs/`

## Notes

- This repository is currently prepared for a private GitHub setup.
- Public release can follow after hardware testing, parameter tuning, and basic
  stability checks.
