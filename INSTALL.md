# Building TuxCards (Qt6 port)

## Requirements

* Qt **6.2+** (recommended 6.5+) with the `Widgets`, `Xml`, `PrintSupport`,
  and `Core5Compat` modules.
* A C++17 compiler (g++ ≥ 9, clang ≥ 10, MSVC 2019+).
* GNU make / nmake / Xcode Build Tools (depending on platform).

## Linux

### Debian / Ubuntu

```bash
sudo apt install qt6-base-dev qt6-base-dev-tools qmake6 \
                 libqt6core5compat6-dev qt6-tools-dev build-essential
```

### Fedora / RHEL

```bash
sudo dnf install qt6-qtbase-devel qt6-qt5compat-devel qt6-qttools-devel \
                 gcc-c++ make
```

### Arch

```bash
sudo pacman -S qt6-base qt6-5compat qt6-tools base-devel
```

### Build

```bash
git clone https://github.com/gzivdo/tuxcards-qt6.git
cd tuxcards-qt6
mkdir build && cd build
qmake6 ../tuxcards.pro
make -j$(nproc)
./tuxcards
```

Install system-wide (optional):

```bash
sudo install -m 755 tuxcards /usr/local/bin/
sudo cp -r ../src/icons /usr/local/share/tuxcards/icons
```

## Windows

1. Install the official Qt 6 SDK from <https://www.qt.io/download-open-source>.
   Pick the **MinGW 64-bit** or **MSVC 64-bit** kit during installation.
2. Open the **"Qt 6.x.x (MinGW 64-bit)"** command prompt from the Start menu.
3. Build:

   ```cmd
   cd tuxcards-qt6
   mkdir build
   cd build
   qmake ..\tuxcards.pro
   mingw32-make -j%NUMBER_OF_PROCESSORS%
   release\tuxcards.exe
   ```

   For MSVC use `nmake` (or `jom`) instead of `mingw32-make`.

4. To produce a self-contained distribution use `windeployqt`:

   ```cmd
   windeployqt release\tuxcards.exe
   ```

## macOS

1. Install Xcode Command Line Tools: `xcode-select --install`.
2. Install Qt 6 via the Qt online installer **or** via Homebrew:

   ```bash
   brew install qt@6
   export PATH="$(brew --prefix qt@6)/bin:$PATH"
   ```

3. Build:

   ```bash
   git clone https://github.com/gzivdo/tuxcards-qt6.git
   cd tuxcards-qt6
   mkdir build && cd build
   qmake ../tuxcards.pro
   make -j$(sysctl -n hw.ncpu)
   open tuxcards.app
   ```

4. To produce a relocatable `.app` bundle:

   ```bash
   macdeployqt tuxcards.app -dmg
   ```

   Drop the resulting `tuxcards.dmg` on another Mac and it should run.

## Packaging

### Debian / Ubuntu (`.deb`)

Skeleton lives under `packaging/debian/`. From the project root:

```bash
sudo apt install devscripts debhelper qt6-base-dev qt6-base-dev-tools \
                 libqt6core5compat6-dev qt6-tools-dev
cp -r packaging/debian ./debian
dpkg-buildpackage -us -uc -b
ls ../tuxcards-qt6_*.deb
sudo dpkg -i ../tuxcards-qt6_*.deb
```

The resulting package installs `/usr/bin/tuxcards`, the icon directory and a
basic `.desktop` entry.

### Fedora / RHEL (`.rpm`)

Skeleton is `packaging/rpm/tuxcards-qt6.spec`.

```bash
sudo dnf install rpmdevtools qt6-qtbase-devel qt6-qt5compat-devel \
                 qt6-qttools-devel gcc-c++ make
rpmdev-setuptree
tar czf ~/rpmbuild/SOURCES/tuxcards-qt6-1.0.0.tar.gz \
        --transform 's,^,tuxcards-qt6-1.0.0/,' \
        -C $(pwd) .
cp packaging/rpm/tuxcards-qt6.spec ~/rpmbuild/SPECS/
rpmbuild -ba ~/rpmbuild/SPECS/tuxcards-qt6.spec
ls ~/rpmbuild/RPMS/x86_64/tuxcards-qt6-*.rpm
```

### AppImage (optional, distro-agnostic)

Build on the *oldest* glibc you want to support, then:

```bash
wget https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage
chmod +x linuxdeployqt-*.AppImage
./linuxdeployqt-*.AppImage build/tuxcards -appimage -qmake=$(which qmake6)
```
