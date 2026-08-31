/*
 *  This file is part of the caQtDM Framework.
 */

#ifndef TST_CA3DCONFIG_H
#define TST_CA3DCONFIG_H

#include <QObject>
#include <QStringList>

class TestCa3DConfig : public QObject
{
    Q_OBJECT

private slots:
    void parsesSceneConfigUtilityFields();
    void parsesLightConfiguration();
    void parsesObjectMasterLinks();
    void parsesCameraPresetsWithoutOverlays();
    void rejectsInvalidUtilityFields();
    void rejectsInvalidLightConfiguration();
    void rejectsInvalidObjectMasterLinks();
    void resolvesFilesFromDisplayPath();
};

class TestCa3DWidget : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void ca3DWidgetBuildsForcedFallbackOverlays();
    void linkedObjectFollowsMasterTranslation();
    void linkedObjectFollowsMasterRotation();
    void linkedObjectAppliesOwnDynamicMotionInMasterSpace();
    void linkedObjectDoesNotInheritMasterScale();
    void configuredOriginPositionRotatesWithObject();

private:
    QStringList thisLibraryPaths;
};

class TestCa3DOverlayWidgetManager : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void ca3DOverlayWidgetManagerLoadsRendersAndTracksDirtyState();
    void ca3DOverlayWidgetManagerForwardsInput();

private:
    QStringList thisLibraryPaths;
};

#endif
