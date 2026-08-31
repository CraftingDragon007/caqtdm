/*
 *  This file is part of the caQtDM Framework.
 */

#ifndef CA3DWIDGET_H
#define CA3DWIDGET_H

#include <QWidget>
#include <QHash>
#include <QList>
#include <QMap>
#include <QMatrix4x4>
#include <QPixmap>
#include <QSet>
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

namespace Qt3DRender {
class QLayer;
class QRenderCapture;
class QRenderCaptureReply;
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

    enum RenderTier {
        RenderTierHardware = 0,
        RenderTierSoftware = 1,
        RenderTierFallback = 2
    };
    Q_ENUM(RenderTier)

    const QString &getSceneConfig() const { return thisSceneConfig; }
    void setSceneConfig(const QString &config);

    bool getFallbackMode() const { return thisFallbackMode; }
    bool getConfigValid() const { return thisConfigValid; }
    const QStringList &getConfigErrors() const { return thisConfigErrors; }
    const ca3DSceneConfig &sceneConfig() const { return thisConfig; }
    int currentCameraPreset() const { return thisCameraPreset; }
    QVector3D currentCameraPosition() const;
    QVector3D currentCameraViewCenter() const;
    QVector3D currentCameraUpVector() const;
    QVector3D currentCameraRotation() const;
    QVector3D currentObjectPosition(const QString &objectId) const;
    QVector3D currentObjectRotation(const QString &objectId) const;
    QVector3D effectiveObjectPosition(const QString &objectId) const;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    QList<QWidget*> overlayRootWidgets() const;
    QString overlayMacro(QWidget *rootWidget) const;
    QString overlayIncludePath(QWidget *rootWidget) const;
    QStringList objectBindingChannels() const;
    void setForce3DPreview(bool enabled);
    int getRenderTier() const { return thisRenderTier; }
    QPixmap grab3DSnapshot(bool includeOverlays = false);
    bool capture3DSnapshot(bool includeOverlays = false);

public slots:
    void setRenderTier(int tier);
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
    void setObjectBindingValue(int bindingIndex, double value);

signals:
    void cameraPositionXChanged(double x);
    void cameraPositionXChanged(int x);
    void cameraPositionYChanged(double y);
    void cameraPositionYChanged(int y);
    void cameraPositionZChanged(double z);
    void cameraPositionZChanged(int z);
    void cameraYawChanged(double yaw);
    void cameraYawChanged(int yaw);
    void cameraPitchChanged(double pitch);
    void cameraPitchChanged(int pitch);
    void overlayWidgetsRebuilt();
    void snapshotCaptured(const QPixmap &pixmap);
    void snapshotCaptureFailed(const QString &error);

private slots:
    void handleSnapshotCaptureCompleted();
    void handleSnapshotCaptureTimeout(quint64 captureToken);

private:
    void updatePlaceholderText();
    void rebuildScene();
    void clearScene();
    void rebuildFallbackView();
    void clearFallbackView();
    void applyFallbackPreset(int preset);
    void emitCameraPositionSignals(const QVector3D &position);
    void emitCameraRotationSignals(double yaw, double pitch);
    bool isDesignerMode() const;
    void setDynamicBindingComponent(const ca3DObjectConfig &object, const ca3DBindingConfig &binding, double value);
    void setLightBindingValue(const ca3DLightConfig &light, const ca3DBindingConfig &binding, double value);
    void rebuildObjectLinks();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    int detectRenderTier() const;
    void maybeInitialize3DView();
    void initialize3DView();
    void update3DViewGeometry();
    void rebuild3DOverlays(Qt3DRender::QLayer *overlayLayer);
    void clear3DOverlays();
    void apply3DOverlayVisibility(int preset);
    void applyCameraPresetConfig(const ca3DCameraPresetConfig &preset);
    void applyObjectTransform(const QString &objectId);
    void applyAllObjectTransforms();
    QMatrix4x4 objectMotionMatrix(const ca3DObjectConfig &object, bool includeDynamic) const;
    QMatrix4x4 effectiveObjectMotionMatrix(const ca3DObjectConfig &object,
                                           QMap<QString, QMatrix4x4> *cache,
                                           QSet<QString> *visiting) const;
    void restoreSnapshotOverlayStates();
    void applyLight(const QString &lightId);
    void issueCaptureRequest(quint64 captureToken);
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
    bool thisFallbackMode;
    bool thisConfigValid;
    bool thisDesignerMode;
    bool thisForce3DPreview;
    int thisRenderTier;
    bool thisRenderTierOverridden;
    bool thisSnapshotCapturePending;
    quint64 thisSnapshotCaptureToken;
    QMap<QString, QVector3D> thisDynamicTranslations;
    QMap<QString, QVector3D> thisDynamicRotations;
    QMap<QString, QVector3D> thisDynamicLightDirections;
    QMap<QString, QVector3D> thisDynamicLightPositions;
    QMap<QString, double> thisDynamicLightIntensities;
    QMap<QString, bool> thisDynamicLightEnabled;
    QHash<QString, int> thisObjectIndexById;
    QHash<QString, QStringList> thisObjectChildrenById;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    Qt3DExtras::Qt3DWindow *this3DView;
    Qt3DCore::QEntity *thisRootEntity;
    QMap<QString, Qt3DCore::QTransform*> thisObjectTransforms;
    QMap<QString, QObject*> this3DLightObjects;
    QMap<QString, Qt3DCore::QTransform*> this3DLightTransforms;
    QMap<QString, Qt3DCore::QEntity*> this3DOverlayEntities;
    QMap<QString, ca3DOverlayWidgetManager*> this3DOverlayManagersById;
    QMap<QString, QObject*> this3DOverlayEventFiltersById;
    QList<ca3DOverlayWidgetManager*> this3DOverlayManagers;
    QList<QObject*> this3DOverlayEventFilters;
    QMap<QString, bool> thisSnapshotOverlayStates;
    Qt3DRender::QRenderCapture *thisRenderCapture;
    Qt3DRender::QRenderCaptureReply *thisPendingCaptureReply;
#endif
};

#endif
