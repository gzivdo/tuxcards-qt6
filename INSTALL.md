# Building TuxCards (Qt6 port)

The project is built with **CMake** (Qt6 + OpenSSL). The legacy
`tuxcards.pro` qmake project, the hand-written `Makefile` and the
`qt-env-*.sh` scripts from the 2010 tree have been removed.

## Requirements

* Qt **6.2+** (recommended 6.5+) with the `Widgets`, `Xml`,
  `PrintSupport`, `Core5Compat`, and `LinguistTools` modules.
* OpenSSL (`libcrypto`) — used for AES-256-GCM file encryption.
* CMake 3.20 or newer.
* A C++17 compiler (g++ ≥ 9, clang ≥ 10, MSVC 2019+).

## Linux

### Debian / Ubuntu

```bash
sudo apt install cmake build-essential libssl-dev libcups2-dev \
                 qt6-base-dev qt6-base-dev-tools \
                 libqt6core5compat6-dev qt6-tools-dev
```

### Fedora / RHEL

```bash
sudo dnf install cmake gcc-c++ make openssl-devel cups-devel \
                 qt6-qtbase-devel qt6-qt5compat-devel qt6-qttools-devel
```

### Arch

```bash
sudo pacman -S cmake base-devel openssl qt6-base qt6-5compat qt6-tools
```

### Build

```bash
git clone https://github.com/gzivdo/tuxcards-qt6.git
cd tuxcards-qt6
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/tuxcards
```

Install system-wide (optional):

```bash
sudo cmake --install build
```

## Windows

1. Install the official Qt 6 SDK from
   <https://www.qt.io/download-open-source>. Pick the **MinGW 64-bit** or
   **MSVC 64-bit** kit.
2. Install OpenSSL: `choco install openssl`.
3. Open the "Qt 6.x.x (MinGW 64-bit)" command prompt from the Start menu.
4. Build:

   ```cmd
   cd tuxcards-qt6
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build --config Release --parallel
   build\Release\tuxcards.exe
   ```

5. For a self-contained distribution:

   ```cmd
   windeployqt build\Release\tuxcards.exe
   ```

## macOS

1. `xcode-select --install`
2. ```bash
   brew install cmake qt@6 openssl@3
   export PATH="$(brew --prefix qt@6)/bin:$PATH"
   ```
3. ```bash
   git clone https://github.com/gzivdo/tuxcards-qt6.git
   cd tuxcards-qt6
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build --parallel
   open build/tuxcards.app
   ```
4. Relocatable `.app` bundle:

   ```bash
   macdeployqt build/tuxcards.app -dmg
   ```

## Running the tests

```bash
cmake -S tests -B tests/build -DCMAKE_BUILD_TYPE=Release
cmake --build tests/build --parallel
ctest --test-dir tests/build --output-on-failure
```

## Packaging

Both `.deb`, `.rpm` and a portable Linux tarball are produced
automatically by GitHub Actions on every `v*` tag — see
`.github/workflows/release.yml`. To build them locally:

### Debian / Ubuntu (`.deb`)

```bash
sudo apt install devscripts debhelper cmake libssl-dev libcups2-dev \
                 qt6-base-dev qt6-base-dev-tools \
                 libqt6core5compat6-dev qt6-tools-dev
cp -r packaging/debian ./debian
dpkg-buildpackage -us -uc -b
```

### Fedora / RHEL (`.rpm`)

```bash
sudo dnf install rpmdevtools cmake openssl-devel cups-devel \
                 qt6-qtbase-devel qt6-qt5compat-devel qt6-qttools-devel
rpmdev-setuptree
VER=3.1.0
tar czf ~/rpmbuild/SOURCES/tuxcards-qt6-${VER}.tar.gz \
        --transform "s,^,tuxcards-qt6-${VER}/," \
        --exclude='.git*' --exclude='build*' .
cp packaging/rpm/tuxcards-qt6.spec ~/rpmbuild/SPECS/
rpmbuild -ba ~/rpmbuild/SPECS/tuxcards-qt6.spec
```

### Portable Linux tarball

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
mkdir tuxcards-qt6-portable
cp build/tuxcards README.md INSTALL.md COPYING CHANGES tuxcards-qt6-portable/
tar -cJf tuxcards-qt6-portable.tar.xz tuxcards-qt6-portable/
```
