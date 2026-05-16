# GUI app — debug symbols are not useful enough to ship a separate
# 3 MB -debuginfo (plus the empty -debugsource) sub-package. Users
# who want a debug build should just configure with -DCMAKE_BUILD_TYPE=
# Debug from source.
# All three knobs are required on modern Fedora; debug_package alone
# is silently overridden by the dist macros, _enable_debug_packages
# disables the subpackage emission, __debug_install_post stops the
# install-post brp-script from extracting symbols in the first place.
%global _enable_debug_packages 0
%global debug_package %{nil}
%global __debug_install_post %{nil}

Name:           tuxcards-qt6
Version:        3.3.0
Release:        1%{?dist}
Summary:        Hierarchical note-taking application (Qt6 port)

License:        GPLv2+
URL:            https://github.com/gzivdo/tuxcards-qt6
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc-c++
BuildRequires:  make
BuildRequires:  cmake
BuildRequires:  qt6-qtbase-devel
BuildRequires:  qt6-qt5compat-devel
BuildRequires:  qt6-qttools-devel
BuildRequires:  openssl-devel
BuildRequires:  cups-devel

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
%cmake
%cmake_build

%install
%cmake_install

%files
%license COPYING
%doc README.md INSTALL.md AUTHORS CHANGES
%{_bindir}/tuxcards
%{_datadir}/applications/tuxcards.desktop
%{_datadir}/icons/hicolor/*/apps/tuxcards.png

%changelog
* Sat May 16 2026 gzivdo <gzivdo@users.noreply.github.com> - 3.0.0-1
- First release of the Qt6 port (3.0.0, after upstream 2.2.1).
