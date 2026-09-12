# Korg drumlogue FX units

Custom effects for the Korg drumlogue, built with the logue SDK v2.

> [!WARNING]
> These units are **pre-release beta software**. They have passed initial tests
> on drumlogue hardware, but they have not yet been tested exhaustively. Keep
> backups of your projects and use the binaries at your own risk.

## Download

Ready-to-install `.drmlgunit` binaries are available on the
[Releases page](https://github.com/sscheidl/drumlogue_fx_units/releases).
Download the latest pre-release and copy only the units you want to use.

## Included effects

### GrainDrift V2 — Delay FX

A tempo-aware granular stereo delay for ghost grains, pitched echoes and
rhythmic glitch clouds. Sixteen rhythm patterns and a wide pitch range can be
shaped from subtle movement into unstable, self-feeding textures.

Parameters: `DENSITY`, `G-LEN`, `RHYTHM`, `FLOW`, `FEED`, `PITCH`, `SPREAD`,
`COLOR` (Clean/Lo-Fi/Dark), `CHAOS`.

### MixForge — Master FX

A compact master-bus processor combining a bipolar DJ-style filter,
compression, four saturation characters and an optional bit crusher. Useful
for quick tonal shaping, glue and deliberately rough final-bus treatment.

Parameters: `FILTER`, `RESONANCE`, `SLOPE` (12 dB/24 dB/Notch), `THRESH`,
`RATIO`, `ATTACK`, `RELEASE`, `DRIVE`, `SAT MODE` (Soft/Hard/Tape/Fold),
`BITCRUSH`, `BITS`.

### TapeVibe — Master FX

A tape-inspired colour box with soft drive, tone and age controls, wow and
flutter modulation, hiss, and probabilistic dropouts. Noise and dropout
characters range from restrained studio tape to worn cassette behaviour.

Parameters: `DRIVE`, `TONE`, `AGE`, `MIX`, `WOW`, `FLUTTER`, `HISS`,
`HISS TYPE`, `DROPOUT`, `DROP TYPE`.

### DrumShuffler — Master FX

A tempo-synchronised micro-delay and stereo-motion effect that pushes a drum
mix away from the grid. Use it for light groove enhancement, shuffled echoes
or wider, more animated rhythmic movement.

Parameters: `SHUFFLE`, `SWING`, `WIDTH`, `MIX`, `DELAY`, `MOD`, `RATE`,
`COLOR`.

### GateVerb80 — Reverb FX

An '80s-inspired gated stereo reverb for punchy drum ambience. Predelay,
high-pass filtering and stereo width help the reverb sit around the dry hit,
while gate and compression controls shape its envelope.

Parameters: `SIZE`, `GATE`, `TONE`, `COMP`, `PREDELAY`, `HP`, `WIDTH`, `MIX`.

## Installation

1. Start the drumlogue in USB mass-storage mode.
2. Copy each `.drmlgunit` file to the matching folder:

   | Unit | Destination on drumlogue |
   | --- | --- |
   | GrainDrift V2 | `Units/DelayFXs/` |
   | MixForge, TapeVibe, DrumShuffler | `Units/MasterFXs/` |
   | GateVerb80 | `Units/ReverbFXs/` |

3. Eject the drumlogue safely and restart it.

The drumlogue loads user units alphabetically. Prefixing a filename with a
number can be used to control load order.

## Beta testing and bug reports

More hardware testers are very welcome. Please open a
[beta bug report](https://github.com/sscheidl/drumlogue_fx_units/issues/new?template=beta-bug-report.yml)
and include:

- drumlogue firmware version;
- affected unit and parameter settings;
- exact steps to reproduce the issue;
- whether the problem survives a restart;
- audio/video evidence when useful.

Particularly useful test areas are long sessions, rapid parameter automation,
extreme parameter combinations, tempo changes, clipping or unexpected level
jumps, and project save/restore behaviour.

## Building from source

The repository contains one PowerShell helper per unit. Each helper copies the
source into a local `logue-sdk/` checkout and invokes the official drumlogue
Docker build environment.

```powershell
.\build_graindrift.ps1
.\build_mixforge.ps1
.\build_tapevibe.ps1
.\build_drumshuffler.ps1
.\build_gateverb80.ps1
```

Requirements: Docker and a checkout of the
[KORG logue SDK](https://github.com/korginc/logue-sdk) at `./logue-sdk`.
GitHub Actions also builds all five binaries from a pinned SDK revision.

## Transparency

Parts of this repository were created with AI-assisted prototyping and light
vibecoding support. DSP ideas, source code and build results are reviewed and
refined manually. Beta feedback is used to identify remaining stability and
sound-quality issues.

This project is independent and is not affiliated with or endorsed by KORG.
