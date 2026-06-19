/*
 *  This file is part of the caQtDM Framework.
 */

#ifndef CA3DWIDGET_H
#define CA3DWIDGET_H

#include <QWidget>
#include <QList>
#include <QMap>
#include <QPixmap>
#include <QString>
#include <QStringList>
#include <qtcontrols_global.h>

#include "ca3dconfig.h"

class QLabel;
class QObject;
class QResizeEvent;
class QShowEvent;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
class ca3DOverlayWidgetManager;

namespace Qt3DCore {
class QEntity;
class QTransform;
}

namespace Qt3DExtras {
class Qt3DWindow;
}
#endif

class QTCON_EXPORT ca3DWidget : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QString sceneConfig READ getSceneConfig WRITE setSceneConfig)
    Q_PROPERTY(bool fallbackMode READ getFallbackMode DESIGNABLE false)
    Q_PROPERTY(bool configValid READ getConfigValid DESIGNABLE false)
    Q_PROPERTY(QStringList configErrors READ getConfigErrors DESIGNABLE false)

public:
    explicit ca3DWidget(QWidget *parent = 0);
    ~ca3DWidget();

    QString getSceneConfig() const { return thisSceneConfig; }
    void setSceneConfig(const QString &config);

    bool getFallbackMode() const { return thisFallbackMode; }
    bool getConfigValid() const { return thisConfigValid; }
    QStringList getConfigErrors() const { return thisConfigErrors; }
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    QList<QWidget*> overlayRootWidgets() const;
    QString overlayMacro(QWidget *rootWidget) const;
    QString overlayIncludePath(QWidget *rootWidget) const;

public slots:
    void setCameraPreset(int preset);
    void setCameraPosition(double x, double y, double z);
    void moveCamera(double dx, double dy, double dz);
    void moveCameraForward(double distance);
    void moveCameraBackward(double distance);
    void moveCameraRight(double distance);
    void moveCameraLeft(double distance);
    void setCameraRotation(double yaw, double pitch);
    void turnCameraUp(double angle);
    void turnCameraDown(double angle);
    void turnCameraRight(double angle);
    void turnCameraLeft(double angle);
    void setCameraViewCenter(double x, double y, double z);
    void setObjectAxisValue(const QString &objectId, const QString &axisId, double value);
    void setObjectTranslation(const QString &objectId, double x, double y, double z);
    void setObjectRotation(const QString &objectId, double rx, double ry, double rz);

signals:
    void overlayWidgetsRebuilt();

private:
    void updatePlaceholderText();
    void rebuildScene();
    void clearScene();
    void rebuildFallbackView();
    void clearFallbackView();
    void applyFallbackPreset(int preset);
    bool isDesignerMode() const;

protected:
    void resizeEvent(QResizeEvent *event);
    void showEvent(QShowEvent *event);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void maybeInitialize3DView();
    void initialize3DView();
    bool shouldUse2DFallback() const;
    void rebuild3DOverlays();
    void clear3DOverlays();
    void apply3DOverlayVisibility(int preset);
    void applyCameraPresetConfig(const ca3DCameraPresetConfig &preset);
    void applyObjectTransform(const QString &objectId);
#endif

    QLabel *thisStatusLabel;
    QWidget *thisViewContainer;
    QWidget *thisFallbackView;
    QLabel *thisFallbackSnapshotLabel;
    QPixmap thisFallbackSnapshotPixmap;
    QMap<QString, QWidget*> thisFallbackOverlayWidgets;
    QMap<QString, QWidget*> thisFallbackOverlayRootWidgets;
    QString thisSceneConfig;
    ca3DSceneConfig thisConfig;
    QStringList thisConfigErrors;
    int thisCameraPreset;
    QSize thisStable3DSize;
    bool thisFallbackMode;
    bool thisConfigValid;
    bool thisDesignerMode;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    Qt3DExtras::Qt3DWindow *this3DView;
    Qt3DCore::QEntity *thisRootEntity;
    QMap<QString, Qt3DCore::QTransform*> thisObjectTransforms;
    QMap<QString, Qt3DCore::QEntity*> this3DOverlayEntities;
    QMap<QString, ca3DOverlayWidgetManager*> this3DOverlayManagersById;
    QMap<QString, QObject*> this3DOverlayEventFiltersById;
    QList<ca3DOverlayWidgetManager*> this3DOverlayManagers;
    QList<QObject*> this3DOverlayEventFilters;
    QMap<QString, QVector3D> thisDynamicTranslations;
    QMap<QString, QVector3D> thisDynamicRotations;
#endif
};

#endif
