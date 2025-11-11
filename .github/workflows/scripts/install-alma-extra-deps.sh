#!/usr/bin/env bash
set -euo pipefail

# This script builds and installs extra dependencies missing from AlmaLinux 10
# Called from the GitHub Actions workflow. Expects PKG_INSTALL_CMD environment
# variable to be set to the distribution package install command (e.g. "dnf install -y").

PKG_INSTALL_CMD=${PKG_INSTALL_CMD:-dnf install -y}

echo "Installing build tools and common deps via: $PKG_INSTALL_CMD"
# dnf-plugins-core is needed for `dnf config-manager` to enable repos
$PKG_INSTALL_CMD --setopt=install_weak_deps=False -y \
  dnf-plugins-core autoconf automake libtool cmake make gcc-c++ git wget pkgconfig \
  openssl-devel libuuid-devel rpmdevtools

echo "Enabled repos:"
dnf repolist --enabled | sed -n '1,200p' || true

# Try to install czmq-devel and cppzmq-devel from EPEL first (packaged)
echo "Attempting to install czmq-devel and cppzmq-devel from EPEL..."
# Prefer packages from EPEL/epel-next if available
if $PKG_INSTALL_CMD --enablerepo=epel,epel-next czmq-devel cppzmq-devel >/dev/null 2>&1; then
  echo "czmq-devel and cppzmq-devel installed from EPEL/epel-next"
else
  echo "czmq-devel/cppzmq-devel not available via dnf in EPEL/epel-next."
  echo "Please enable EPEL/epel-next or provide these packages (e.g. install manually or add a repository)."
  exit 1
fi

# Verify cppzmq header is present after the package installation
if [ ! -f /usr/include/zmq.hpp ] && [ ! -f /usr/local/include/zmq.hpp ]; then
  echo "cppzmq header (zmq.hpp) not found after package install."
  echo "Ensure cppzmq-devel/cppzmq is installed or place zmq.hpp under /usr/include or /usr/local/include."
  exit 1
fi

# Build or install Qwt as an RPM for reproducibility
if ! rpm -q qwt-qt6 >/dev/null 2>&1 && ! rpm -q qwt >/dev/null 2>&1; then
  echo "qwt package not found; attempting to build Qwt RPM via .github/workflows/scripts/build-qwt-rpm.sh"
  if [ -x ./.github/workflows/scripts/build-qwt-rpm.sh ]; then
    ./.github/workflows/scripts/build-qwt-rpm.sh
    # Install resulting RPMs (if any)
    if compgen -G "$HOME/rpmbuild/RPMS/*/qwt*" >/dev/null; then
      rpm -Uvh $HOME/rpmbuild/RPMS/*/qwt*.rpm || true
    else
      echo "Qwt RPM build completed but no RPMs were found in $HOME/rpmbuild/RPMS/."
      echo "Check build logs and ensure QWT_VER is set and the source tarball exists in rpmbuild SOURCES."
      exit 1
    fi
  else
    echo "build-qwt-rpm.sh missing or not executable."
    echo "Please add .github/workflows/scripts/build-qwt-rpm.sh to the repository or install qwt/qwt-qt6 from a package repository."
    exit 1
  fi
else
  echo "qwt already installed; skipping Qwt build."
fi

echo "AlmaLinux extra deps installation complete."
