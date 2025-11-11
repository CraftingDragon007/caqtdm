#!/usr/bin/env bash
#set -x
set -euo pipefail

# Builds a Qwt RPM (Qt6-compatible) in the user's rpmbuild tree.
# Resulting RPMs appear under ~/rpmbuild/RPMS/ and will be installed by the caller.

QWT_VER=${QWT_VER:-6.3.0}
RPMBUILD_DIR=${RPMBUILD_DIR:-$HOME/rpmbuild}

mkdir -p "$RPMBUILD_DIR"/{SOURCES,SPECS,BUILD,RPMS,SRPMS}

pushd "$RPMBUILD_DIR/SOURCES" >/dev/null
# Use the specified CodeForge uplink to fetch the requested tag and create
# the source tarball expected by the spec file.
QWT_GIT_REPO=${QWT_GIT_REPO:-https://git.code.sf.net/p/qwt/git}
if [ ! -f qwt-${QWT_VER}.tar.bz2 ]; then
  if ! command -v git >/dev/null 2>&1; then
    echo "git is required to fetch Qwt by tag but was not found; exiting"
    exit 1
  fi

  QWT_SRC_DIR="$RPMBUILD_DIR/SOURCES/qwt-${QWT_VER}"
  if [ -d "$QWT_SRC_DIR" ] && [ -n "$(ls -A "$QWT_SRC_DIR")" ]; then
    echo "Using existing checkout at $QWT_SRC_DIR"
  else
    echo "Cloning $QWT_GIT_REPO tag v${QWT_VER} into $QWT_SRC_DIR"
    # Try cloning using the v-prefixed tag first (common), fall back to non-v.
    if ! git clone --branch "v${QWT_VER}" --depth 1 "$QWT_GIT_REPO" "$QWT_SRC_DIR" >/dev/null 2>&1; then
      echo "Clone with tag v${QWT_VER} failed, trying tag ${QWT_VER}"
      if ! git clone --branch "${QWT_VER}" --depth 1 "$QWT_GIT_REPO" "$QWT_SRC_DIR" >/dev/null 2>&1; then
        echo "Failed to clone tag v${QWT_VER} from $QWT_GIT_REPO"
        echo "Ensure the tag exists in the repository or set QWT_GIT_REPO to a different URL."
        exit 1
      fi
    fi
  fi

  echo "Creating tarball qwt-${QWT_VER}.tar.bz2 from git checkout"
  tar -cjf "qwt-${QWT_VER}.tar.bz2" -C "$RPMBUILD_DIR/SOURCES" "qwt-${QWT_VER}"
fi
popd >/dev/null

SPECFILE="$RPMBUILD_DIR/SPECS/qwt-qt6.spec"
cat > "$SPECFILE" <<EOF
Name:           qwt
Version:        ${QWT_VER}
Release:        1%{?dist}
Summary:        Qt Widgets for Technical Applications (Qwt built for Qt6)
License:        LGPLv2 with exceptions
URL:            http://qwt.sourceforge.net/
Source0:        qwt-${QWT_VER}.tar.bz2
BuildRoot:      %{_tmppath}/%{name}-%{version}-build
BuildRequires:  make

%description
The Qwt library contains GUI Components and utility classes which are primarily
useful for programs with a technical background.
Besides a 2D plot widget it provides scales, sliders, dials, compasses,
thermometers, wheels and knobs to control or display values, arrays
or ranges of type double.

%package doc
Summary: Developer documentation for %{name}
BuildArch: noarch
%description doc
%{summary}.

%package qt6
Summary: Qt6 Widgets for Technical Applications
BuildRequires: pkgconfig(Qt6Concurrent) pkgconfig(Qt6PrintSupport) pkgconfig(Qt6Widgets)
BuildRequires: pkgconfig(Qt6OpenGL) pkgconfig(Qt6Svg)
BuildRequires: pkgconfig(Qt6Designer)
Provides: qwt-qt6 = %{version}-%{release}
Provides: qwt-qt6%{_isa} = %{version}-%{release}
%description qt6
%{summary}.

%package qt6-devel
Summary:  Development files for %{name}-qt6
Provides: qwt-qt6-devel = %{version}-%{release}
Provides: qwt-qt6-devel%{_isa} = %{version}-%{release}
Requires: %{name}-qt6%{?_isa} = %{version}-%{release}
%description qt6-devel
%{summary}.

%prep
%autosetup -p1

%build

%{qmake_qt6} QWT_CONFIG+=QwtPkgConfig
make -j"$(nproc)"

%install
rm -rf %{buildroot}
make install INSTALL_ROOT=%{buildroot} -C %{_target_platform}-qt6

mkdir -p %{buildroot}%{_defaultdocdir}/%{name}
mkdir -p %{buildroot}%{_mandir}

rm -rf %{buildroot}%{_defaultdocdir}/%{name}/html %{buildroot}%{_mandir}/*
mv %{buildroot}%{_qt6_docdir}/html/html %{buildroot}%{_defaultdocdir}/%{name}/html
mv %{buildroot}%{_qt6_docdir}/html/man/man3 %{buildroot}%{_mandir}/

%files doc
%doc %{_defaultdocdir}/%{name}/
%{_mandir}/man3/*

%files qt6
%license COPYING
%doc README
%{_qt6_libdir}/libqwt-qt6.so.6*

%files qt6-devel
%{_qt6_headerdir}/qwt/
%{_qt6_libdir}/libqwt-qt6.so
%{_qt6_libdir}/pkgconfig/Qt6Qwt6.pc
%{_qt6_archdatadir}/mkspecs/features/qwt*
%{_qt6_plugindir}/designer/libqwt_designer_plugin.so

%changelog
* $(date +"%a %b %d %Y") caQtDM Team - ${QWT_VER}-1
- Built Qwt ${QWT_VER} for Qt6 (added -devel subpackage)
EOF

# Ensure the source tarball exists in SOURCES (fail early if missing)
if [ ! -f "$RPMBUILD_DIR/SOURCES/qwt-${QWT_VER}.tar.bz2" ]; then
  echo "Source archive qwt-${QWT_VER}.tar.bz2 not found in $RPMBUILD_DIR/SOURCES/."
  echo "Please download it into $RPMBUILD_DIR/SOURCES/ or set QWT_VER to a version you have available."
  exit 1
fi

# Build the RPM
rpmbuild -ba "$SPECFILE"

echo "Built Qwt RPMs placed in $RPMBUILD_DIR/RPMS/"