Name:           tuxcards-qt6
Version:        3.1.0
Release:        1%{?dist}
Summary:        Hierarchical note-taking application (Qt6 port)

License:        GPLv2+
URL:            https://github.com/gzivdo/tuxcards-qt6
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc-c++
BuildRequires:  make
BuildRequires:  qt6-qtbase-devel
BuildRequires:  qt6-qt5compat-devel
BuildRequires:  qt6-qttools-devel

Requires:       qt6-qtbase
Requires:       qt6-qt5compat

%description
TuxCards is a Qt-based personal information manager / tree-based notebook
with rich-text editing, per-entry icons and colors, search, HTML export
and optional Blowfish encryption.

This package is a Qt6 port of the original TuxCards 2.0 / 2010.06.1,
with the SideBar and PNG icons back-ported from TuxCards 2.2.1.

%prep
%setup -q

%build
mkdir -p build
cd build
qmake6 ../tuxcards.pro
%make_build

%install
install -D -m 755 build/tuxcards %{buildroot}%{_bindir}/tuxcards
install -D -m 644 packaging/desktop/tuxcards.desktop \
        %{buildroot}%{_datadir}/applications/tuxcards.desktop
install -D -m 644 src/icons/lo32-app-tuxcards.png \
        %{buildroot}%{_datadir}/icons/hicolor/32x32/apps/tuxcards.png
install -D -m 644 src/icons/lo16-app-tuxcards.png \
        %{buildroot}%{_datadir}/icons/hicolor/16x16/apps/tuxcards.png

%files
%license COPYING
%doc README.md INSTALL.md AUTHORS CHANGES
%{_bindir}/tuxcards
%{_datadir}/applications/tuxcards.desktop
%{_datadir}/icons/hicolor/*/apps/tuxcards.png

%changelog
* Sat May 16 2026 gzivdo <gzivdo@users.noreply.github.com> - 3.0.0-1
- First release of the Qt6 port (3.0.0, after upstream 2.2.1).
