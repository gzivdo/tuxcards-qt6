# TuxCards (Qt6 port)

[![Build](https://github.com/gzivdo/tuxcards-qt6/actions/workflows/build.yml/badge.svg)](https://github.com/gzivdo/tuxcards-qt6/actions/workflows/build.yml)
[![Release](https://img.shields.io/github/v/release/gzivdo/tuxcards-qt6)](https://github.com/gzivdo/tuxcards-qt6/releases)
[![License](https://img.shields.io/badge/License-GPL%20v2%2B-blue.svg)](COPYING)

Hierarchical note-taking application for Linux, Windows and macOS.
A tree-based journal / personal wiki with rich-text editing, optional
Blowfish file encryption, HTML export and sub-tree color tags.

This is a **Qt6 fork** of TuxCards. It builds and runs on modern
distributions where the original Qt3/Qt4 versions no longer compile.

```
Original project:  http://www.tuxcards.de
Source tarballs:   http://www.tuxcards.de/src/  (canonical upstream)
This fork:         https://github.com/gzivdo/tuxcards-qt6
```

## Lineage

- TuxCards 1.x / 2.0 (Alexander Theel, 2000–2004) — Qt3.
- TuxCards 2.0 / 2010.06.1 (Amit Chaudhary, 2006–2010) — added encryption,
  released against Qt 3 with Qt3Support shim.
- TuxCards 2.2.1 (Theel, 2010s) — Qt 4 rewrite published at
  <https://www.tuxcards.de/src/tuxcards-2.2.1/tuxcards-2.2.1.tar.gz>.
- **TuxCards Qt6 port** (this repo, 2026) — Qt6 migration of the 2010.06.1
  codebase, with the SideBar (`CColorBar`) and PNG icons back-ported from
  2.2.1.

License: **GPL v2 or later** — same as upstream (see [COPYING](COPYING)).

## Features preserved from upstream

* Tree of notes with per-entry icons, font and color (also per sub-tree).
* Rich-text editor: bold/italic/underline, alignment, bullet / ordered lists,
  font family / size combo.
* Search across the whole tree or the current sub-tree (case-sensitive,
  titles only), with "More" button revealing scope selection.
* Blowfish encryption per entry and per file.
* HTML export of the entire collection.
* Configurable left-side `CColorBar` (gradient + horizontal/vertical
  captions) — re-introduced from 2.2.1.
* Configuration dialog: General / SideBar / Tree font / Editor font.

## Building from source

See [INSTALL.md](INSTALL.md) for full per-platform instructions
(Linux, Windows, macOS, plus `.deb` and `.rpm` packaging).

Quick build on Debian/Ubuntu:

```bash
sudo apt install qt6-base-dev qt6-base-dev-tools qmake6 \
                 libqt6core5compat6-dev qt6-tools-dev build-essential
mkdir build && cd build
qmake6 ../tuxcards.pro
make -j$(nproc)
./tuxcards
```

## Credits

* Alexander Theel — original author of TuxCards.
* Amit Chaudhary — TuxCards 2.0 maintainer, encryption work.
* Yahoo! Inc. — contributions in 2007.
* gzivdo — initiator of the Qt6 port (2026).
* Claude Opus 4.7 (Anthropic) — performed the Qt6 migration.
