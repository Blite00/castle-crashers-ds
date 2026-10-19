# Notice: Castle Crashers content and this repository

**Castle Crashers DS is an unofficial fan project.** It is not affiliated with,
endorsed by, sponsored by or supported by The Behemoth. *Castle Crashers*, its
names, characters, logos, art, animation, music, sound, levels and data are the
property of The Behemoth.

## What is in this repository

Only original work: the Nintendo DS engine (`source/`, `arm7/`), the conversion
tools (`tools/`), tests (`tests/`) and documentation (`docs/`). It contains no
files from Castle Crashers and no content converted from it. Where the code
refers to game content, it does so by identifier (an archive name, a sprite or
frame number), so the content itself is read from the player's own copy at
setup time. `data/frozen/` holds a few small numeric tables (for example a
palette chosen once) that the converters read; they contain no artwork, audio
or text.

`tools/audit_clean.py` enforces this: it rejects images, audio, Flash files,
archives, ROMs and converted files, and, given a game folder, rejects any file
that matches a game file or contains a run of the game's text. It runs on every
commit (when the hook is installed) and in CI.

## The archive extractor is not included

Castle Crashers keeps its data in encrypted `.pak` archives. Reading them needs
the decryption routine published by Ethan Roseman in
[ethteck/castlecrashers](https://github.com/ethteck/castlecrashers) (MIT
License). Because that routine contains the game's archive keys, this
repository does **not** include it. `tools/setup_assets.py` downloads one
pinned version of `decrypt_asset.py` (checked by SHA-256) into `tools/vendor/`,
which is git-ignored, and the `blowfish` package from PyPI. The audit refuses to
let either file be committed.

Whether decrypting your own copy of a game for personal interoperability is
permitted depends on where you live. In some countries, laws against
circumventing technical protection measures (for example the DMCA in the United
States) can apply even to people who own the game. Check what applies to you
before you run the setup. Maintainers of forks should keep the extractor out of
their repositories too.

## Third-party tools fetched by the setup

- [JPEXS Free Flash Decompiler](https://github.com/jindrapetrik/jpexs-decompiler) 22.0.2, GPL-3.0, downloaded into `.tools/ffdec/` when no copy is found.
- [`decrypt_asset.py`](https://github.com/ethteck/castlecrashers), MIT, Copyright (c) 2022 Ethan Roseman.
- [`blowfish`](https://pypi.org/project/blowfish/) 0.6.1, GPL-3.0, installed with pip.

They are separate programs, run as tools; none of them is distributed with this
repository.

## What the setup creates on your computer

`tools/setup_assets.py` reads the `data/` folder of a copy of Castle Crashers
that you obtained legitimately, converts the parts the DS needs, and builds
`castle-crashers-ds.nds`. The results (`assets/`, `generated/`, `nitrofiles/`,
`.game-install/` and the `.nds` ROM) contain The Behemoth's content. They are
for your own personal use with your own copy of the game. **Do not upload,
share, sell or distribute them**, including in issues, pull requests, releases,
forks or download sites.

The GitHub releases of this project, if any, contain source code only.

## Requests from the rights holder

If The Behemoth asks for any change to, or the removal of, this project, the
maintainers will comply. Contact: open an issue, or reach the maintainer listed
on the repository page.

This notice is not legal advice.
