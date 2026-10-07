#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"

mkdir -p "$CAQTDM_COLLECT"
cp "$CAQTDM_BUILD/caQtDM_Viewer/release/caQtDM.exe" "$CAQTDM_COLLECT/"
cp "$CAQTDM_BUILD/caQtDM_Lib/release/caQtDM_Lib.dll" "$CAQTDM_COLLECT/"
cp "$CAQTDM_BUILD/caQtDM_QtControls/release/qtcontrols.dll" "$CAQTDM_COLLECT/"
cp "$QWTLIB/qwt.dll" "$CAQTDM_COLLECT/"

for dll in ca Com dbCore dbRecStd nt pvAccess pvAccessCA pvAccessIOC \
    pvaClient pvData pvDatabase qsrv; do
    cp "$EPICS_BASE/bin/$EPICS_HOST_ARCH/$dll.dll" "$CAQTDM_COLLECT/"
done

windeployqt --release --compiler-runtime --no-translations --no-opengl-sw \
    --dir "$CAQTDM_COLLECT" \
    "$CAQTDM_COLLECT/caQtDM.exe" "$CAQTDM_COLLECT/caQtDM_Lib.dll" \
    "$CAQTDM_COLLECT/qtcontrols.dll" "$CAQTDM_COLLECT/qwt.dll"

bash "$REACTOS_PACKAGE_DIR/check-deployment.sh"
