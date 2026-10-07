#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"

mkdir -p "$CAQTDM_COLLECT"
mkdir -p "$CAQTDM_COLLECT/designer"
for group in controllers graphics monitors utilities; do
    cp "$CAQTDM_BUILD/caQtDM_QtControls/plugins/release/qtcontrols_${group}_plugin.dll" \
        "$CAQTDM_COLLECT/designer/"
done
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
    "$CAQTDM_COLLECT/qtcontrols.dll" "$CAQTDM_COLLECT/qwt.dll" \
    "$CAQTDM_COLLECT/designer/qtcontrols_controllers_plugin.dll" \
    "$CAQTDM_COLLECT/designer/qtcontrols_graphics_plugin.dll" \
    "$CAQTDM_COLLECT/designer/qtcontrols_monitors_plugin.dll" \
    "$CAQTDM_COLLECT/designer/qtcontrols_utilities_plugin.dll"

bash "$REACTOS_PACKAGE_DIR/check-deployment.sh"
