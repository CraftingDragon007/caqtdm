/*
 *  This file is part of the caQtDM Framework.
 */

#ifndef TST_CA3DCONFIG_H
#define TST_CA3DCONFIG_H

#include <QObject>

class TestCa3DConfig : public QObject
{
    Q_OBJECT

private slots:
    void parsesSceneConfigUtilityFields();
    void rejectsInvalidUtilityFields();
    void resolvesFilesFromDisplayPath();
};

class TestCa3DWidget : public QObject
{
    Q_OBJECT

private slots:
    void ca3DWidgetBuildsForcedFallbackOverlays();
};

class TestCa3DOverlayWidgetManager : public QObject
{
    Q_OBJECT

private slots:
    void ca3DOverlayWidgetManagerLoadsRendersAndTracksDirtyState();
    void ca3DOverlayWidgetManagerForwardsInput();
};

#endif
