# TuxCards (Qt6 port)

[![Build](https://github.com/gzivdo/tuxcards-qt6/actions/workflows/build.yml/badge.svg)](https://github.com/gzivdo/tuxcards-qt6/actions/workflows/build.yml)
[![Release](https://img.shields.io/github/v/release/gzivdo/tuxcards-qt6)](https://github.com/gzivdo/tuxcards-qt6/releases)
[![License](https://img.shields.io/badge/License-GPL%20v2%2B-blue.svg)](COPYING)

Hierarchical note-taking application for Linux, Windows and macOS —
a tree-based journal / personal wiki with rich-text editing, optional
AES-256-GCM file encryption, HTML and Markdown export, and per-entry
icons and colors.

This is a **Qt6 fork** of TuxCards. It builds and runs on modern
distributions where the original Qt3/Qt4 versions no longer compile.

```
Original project:  http://www.tuxcards.de
Source tarballs:   http://www.tuxcards.de/src/
This fork:         https://github.com/gzivdo/tuxcards-qt6
```

## Lineage

- TuxCards 1.x / 2.0 (Alexander Theel, 2000–2004) — Qt3.
- TuxCards 2.0 / 2010.06.1 (Amit Chaudhary, 2006–2010) — added Blowfish
  encryption, last release against Qt 3 with the Qt3Support shim.
- TuxCards 2.2.1 (Theel, 2010s) — Qt 4 rewrite at
  <https://www.tuxcards.de/src/tuxcards-2.2.1/tuxcards-2.2.1.tar.gz>.
- **TuxCards Qt6 port** (this repo, 2026) — Qt6 migration of the
  2010.06.1 codebase, with the SideBar (`CColorBar`), PNG icons,
  Bookmarks toolbar and reworked RecentFileList back-ported from 2.2.1.

License: **GPL v2 or later** — same as upstream (see [COPYING](COPYING)).

## Features

* Tree of notes with per-entry icons, font and color (also per sub-tree).
* Rich-text editor: bold/italic/underline, alignment, bullet / ordered
  lists, font family / size combo, image insertion.
* In-entry **find & replace** bar (Ctrl+F, Ctrl+H; F3 / Shift+F3 for
  next / previous; case-sensitive and whole-word toggles).
* Search across the whole tree or the current sub-tree
  (Ctrl+Shift+F, or F7 if your WM eats the Ctrl+Shift combo), with the
  matched substring highlighted in bold in the results.
* **Print** the current entry, with a separate **Print preview**
  action (Ctrl+Shift+P).
* HTML and Markdown export of entries; Markdown import.
* Configurable left-side `CColorBar` (gradient + horizontal/vertical
  captions) — back-ported from 2.2.1.
* Bookmarks bar pinned at the bottom of the window, persisted between
  sessions.
* Auto-save indicator in the status bar.
* Full UI translation to Russian; English, German, Chinese (Simplified
  and Traditional), Spanish, French, Brazilian Portuguese, Japanese and
  Korean are wired up as `.ts` files awaiting community translations.
* File encryption with a choice of two modern AEADs:
  * **AES-256-GCM + PBKDF2-SHA256** (OpenSSL backend, default), and
  * **XChaCha20-Poly1305 + Argon2i** (vendored monocypher, no external
    dependency — useful for Windows/macOS builds with no system OpenSSL).
  One binary reads any format: AES-GCM, XChaCha, or the legacy
  Blowfish + MD5 from TuxCards 2.0. Algorithm choice and "re-encrypt
  all on format change" toggle live in **Options → Encryption**.

## What's new in the Qt6 port

Relative to the last upstream releases (Theel 2.2.1 / Chaudhary 2.0):

* Builds and runs on modern Linux, Windows and macOS with **Qt 6**.
  CMake-only build (the old `tuxcards.pro` / Makefile / `qt-env-*.sh`
  helpers are gone). Qt3 / Qt3Support legacy stamped out.
* **Multi-backend file crypto** — AES-256-GCM via OpenSSL *and*
  XChaCha20-Poly1305 + Argon2i via vendored monocypher; either can be
  the default-write backend at build time, both auto-detected on read.
  The legacy Blowfish + MD5 format from TuxCards 2.0 is still readable.
* **Save no longer freezes the UI** on encrypted files. A session-wide
  key cache means the KDF runs once per save / load burst instead of
  once per element. Dirty-tracking emits untouched entries
  byte-for-byte, so saves of encrypted files are effectively free.
* **Bookmarks bar** pinned at the bottom of the window
  (back-port from 2.2.1).
* **Markdown** export and import per entry; **HTML** export of the
  whole tree.
* Editor toolbar with font / size combos, bullet & ordered lists, text
  color, image insertion.
* Search results highlight the matched substring in bold.
* CI builds `.deb` per supported Debian/Ubuntu release, `.rpm` for
  Fedora, portable `tar.xz` for Linux, `.dmg` for macOS and `.zip`
  for Windows. Windows and macOS each have two flavours: full
  (OpenSSL + monocypher) and `-noopenssl` (monocypher only, smaller).
* Russian UI fully translated; nine other languages stubbed.

## Building

See [INSTALL.md](INSTALL.md) for full per-platform instructions
(Linux, Windows, macOS, plus `.deb` and `.rpm` packaging).

Quick build on Debian/Ubuntu:

```bash
sudo apt install cmake build-essential libssl-dev libcups2-dev \
                 qt6-base-dev qt6-base-dev-tools \
                 libqt6core5compat6-dev qt6-tools-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/tuxcards
```

## Credits

* Alexander Theel — original author of TuxCards.
* Amit Chaudhary — TuxCards 2.0 maintainer, encryption work.
* Yahoo! Inc. — contributions in 2007.
* gzivdo — initiator of the Qt6 port (2026).
* Claude Opus 4.7 (Anthropic) — performed the Qt6 migration.
