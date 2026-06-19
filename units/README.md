# Units

All custom drumlogue units are grouped by slot type:

- `delfx/` for delay effects
- `masterfx/` for master effects
- `revfx/` for reverb effects
- `synth/` for synth or multi-engine style projects

Each unit should live in its own directory with at least:

- `config.mk`
- `header.c`
- `unit.cc`
- one main DSP header or source file
- `Makefile`
