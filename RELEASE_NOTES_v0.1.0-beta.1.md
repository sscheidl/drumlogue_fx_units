# drumlogue FX units v0.1.0-beta.1

This is the first public beta binary release of five custom effects for the
Korg drumlogue. Initial hardware smoke tests passed, but the collection is
still pre-release software. Feedback and reproducible bug reports are very
welcome.

## Included binaries

- **GrainDrift V2** (`taureon_graindrift.drmlgunit`) — tempo-aware granular
  stereo delay with rhythm patterns, pitch shifting, feedback and three colour
  modes.
- **MixForge** (`taureon_mixforge.drmlgunit`) — master-bus filter, compressor,
  saturation and optional bit crusher.
- **TapeVibe** (`taureon_tapevibe.drmlgunit`) — tape-style drive, tone, age,
  wow/flutter, hiss and dropouts.
- **DrumShuffler** (`taureon_drumshuffler.drmlgunit`) — tempo-synchronised
  micro-delay, shuffle and stereo motion for drum grooves.
- **GateVerb80** (`taureon_gateverb80.drmlgunit`) — gated stereo reverb with
  predelay, tone, high-pass filtering, compression and width.

## Installation

Copy the binaries to the matching folders while the drumlogue is in USB
mass-storage mode:

- GrainDrift V2: `Units/DelayFXs/`
- MixForge, TapeVibe and DrumShuffler: `Units/MasterFXs/`
- GateVerb80: `Units/ReverbFXs/`

Safely eject the device and restart it.

## Beta testing

Please report problems with the
[beta bug-report form](https://github.com/sscheidl/drumlogue_fx_units/issues/new?template=beta-bug-report.yml)
and include the drumlogue firmware version, unit settings and exact reproduction steps. Long sessions,
tempo changes, rapid parameter changes, extreme settings, unexpected clipping
or level jumps, and project save/restore are especially valuable test cases.

The ZIP contains all five binaries, file-type verification and SHA-256
checksums. Individual binaries are also attached for selective installation.

This project is independent and is not affiliated with or endorsed by KORG.
