#!/usr/bin/env bash
# Source this file from an MSYS2 Bash shell before every build.

REACTOS_PACKAGE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export CAQTDM_SOURCE="$(cygpath -m "${CAQTDM_SOURCE:-$REACTOS_PACKAGE_DIR/../../..}")"
export QTHOME="$(cygpath -m "${QTHOME:-/c/Qt57/5.7/mingw53_32}")"
export MINGW_HOME="$(cygpath -m "${MINGW_HOME:-/c/Qt57/Tools/mingw530_32}")"
export QWTHOME="$(cygpath -m "${QWTHOME:-$HOME/qwt-git}")"
export EPICS_BASE="$(cygpath -m "${EPICS_BASE:-$HOME/epics-base}")"
export REACTOS_CRT_DIR="$(cygpath -m "${REACTOS_CRT_DIR:-/c/Qt57/reactos-compat}")"
export CAQTDM_BUILD="$(cygpath -m "${CAQTDM_BUILD:-$HOME/caqtdm-reactos-build}")"
export CAQTDM_COLLECT="$(cygpath -m "${CAQTDM_COLLECT:-$HOME/caqtdm-reactos-collect}")"

export PATH="$(cygpath -u "$MINGW_HOME")/bin:$(cygpath -u "$QTHOME")/bin:$PATH"
export EPICS_HOST_ARCH=win32-x86-mingw
export EPICSINCLUDE="$EPICS_BASE/include"
export EPICSLIB="$EPICS_BASE/lib/$EPICS_HOST_ARCH"
export QWTINCLUDE="$QWTHOME/src"
export QWTLIB="$QWTHOME/lib"
export QWTLIBNAME=qwt
export QWTVERSION=6.3.0
export QTCONTROLS_LIBS="$CAQTDM_COLLECT"
export QTBASE="$CAQTDM_COLLECT"
