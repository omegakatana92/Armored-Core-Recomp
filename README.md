# Armored Core Recomp

Static recompilation project for **Armored Core (USA)** on the original PlayStation, built with **PSXRecomp** and **recomp-ui**.

> **Important:** This repository does not contain the original game, disc images, BIOS files, extracted game executables, generated game code, or other copyrighted game assets. You must provide your own legally obtained copy of Armored Core.

## Game metadata

| Field | Value |
|---|---|
| Title | Armored Core |
| Region | USA |
| Serial | `SCUS-94182` |
| Boot executable | `SCUS_941.82` |
| Players | 2 |
| Publisher | Sony Computer Entertainment America |
| Year | 1997 |
| Disc count | 1 |

The project configuration records the verified executable load address, entry point, text size, disc fingerprints, and seed addresses needed by the recompilation workflow.

## Framework

This project uses:

- **PSXRecomp** — PlayStation static recompilation framework created and developed by **mstan**.
- **recomp-ui** — launcher/setup UI from RetroPortingToolKit.

Current framework repository: https://github.com/RetroPortingToolKit/psxrecomp

Special thanks and full credit to **mstan** for creating and developing **PSXRecomp**, the PlayStation static recompilation tool and framework that makes this project possible.

Additional thanks to the PSXRecomp contributors, RetroPortingToolKit, and recomp-ui contributors.

## Getting started

Clone the repository and initialize its framework submodules:

```bash
git clone --recursive https://github.com/omegakatana92/Armored-Core-Recomp.git
cd Armored-Core-Recomp
git submodule update --init --recursive
```

Provide your own legally obtained **Armored Core (USA)** disc dump locally. The setup/generation workflow uses that local copy to prepare and generate the files required for your personal build. Those generated and extracted files are intentionally excluded from this repository.

A typical development build uses the current PSXRecomp workflow:

```bash
python3 psxrecomp/psxrecomp_cli.py generate --config game.toml --project-root . --disc "<path-to-your-legal-cue>"
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --target psx-runtime
```

Refer to the PSXRecomp documentation in the submodule for the current platform-specific setup and generation requirements.

## Repository safety policy

The public repository intentionally excludes:

- `.bin`, `.cue`, `.iso`, `.chd`, `.img`, `.pbp`, and other ROM/disc-image formats
- PlayStation BIOS dumps
- `.exe`, `.dll`, and other compiled binaries
- `build/`, `build-*`, CMake output, and release staging directories
- the extracted `SCUS_941.82` PlayStation executable
- generated recompilation C derived from the original game executable
- saves, FMV, audio, textures, models, artwork, and other extracted game assets

The repository contains only project source/configuration, metadata, symbol/seed information, documentation, and framework submodule references appropriate for public development.

## Legal

Armored Core is the property of its respective rights holders. This is an unofficial fan and technical research project and is not affiliated with or endorsed by FromSoftware, Sony, mstan, RetroPortingToolKit, or other referenced projects.
